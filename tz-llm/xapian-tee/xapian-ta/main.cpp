#include <xapian.h>
#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <sstream>

std::vector<std::string> secure_docs;
const char XOR_KEY = 0x5A;

std::string xor_cipher(const std::string& input) {
    std::string out = input;
    for (char& c : out) c ^= XOR_KEY;
    return out;
}

typedef int cap_t;

extern "C" {
    struct smc_registers {
        unsigned long x0;
        unsigned long x1;
        unsigned long x2;
        unsigned long x3;
        unsigned long x4;
    };
    unsigned long usys_tee_wait_switch_req(struct smc_registers *regs);
    unsigned long usys_tee_switch_req(struct smc_registers *regs);
    int usys_tee_create_ns_pmo(unsigned long paddr, unsigned long size);
    void *chcore_auto_map_pmo(int pmo_cap, unsigned long size, unsigned long perm);
}

#define VMR_READ (1 << 0)
#define VMR_WRITE (1 << 1)

#define SHM_SIZE (1 * 1024 * 1024) // 1MB for query/result

int main() {
    std::cout << "[Xapian-TA] Starting inside TEE! (Version 2.0 - Service Loop Enabled)" << std::endl;

    // 1. Initialize Xapian Database (In-Memory to avoid unsupported 'socketpair' syscalls in TEE)
    Xapian::WritableDatabase db = Xapian::InMemory::open();
    Xapian::TermGenerator termgenerator;
    termgenerator.set_stemmer(Xapian::Stem("en"));

    secure_docs.push_back("This is a secret document stored in TrustZone.");
    secure_docs.push_back("Another top secret AI model data.");
    secure_docs.push_back("Welcome to Xapian-TA Version 2.0! The infinite loop bug is fixed.");

    for (const auto& d : secure_docs) {
        Xapian::Document doc;
        doc.set_data(d);
        termgenerator.set_document(doc);
        termgenerator.index_text(d);
        db.add_document(doc);
    }

    db.commit();

    std::cout << "[Xapian-TA] Indexed " << db.get_doccount() << " documents in memory." << std::endl;

    // 2. Wait for SMC from Linux (Normal World)
    struct smc_registers req = {0};
    std::cout << "[Xapian-TA] Waiting for SMC from Normal World..." << std::endl;
    unsigned long paddr = usys_tee_wait_switch_req(&req);
    
    std::cout << "[Xapian-TA] Received SMC! paddr: " << std::hex << paddr << std::dec << std::endl;

    // 3. Map Shared Memory
    cap_t pmo = usys_tee_create_ns_pmo(paddr, SHM_SIZE);
    void* vaddr = chcore_auto_map_pmo(pmo, SHM_SIZE, VMR_READ | VMR_WRITE);

    if (vaddr) {
        char* shm_buf = (char*)vaddr;
        // Write handshake marker required by tzdriver
        sprintf(shm_buf, "msg from tee\n");

        std::cout << "[Xapian-TA] Initialization complete. Entering service loop..." << std::endl;

        while (1) {
            // Yield to Linux and wait for the next actual query
            std::cout << "[Xapian-TA] Yielding and waiting for query..." << std::endl;
            usys_tee_wait_switch_req(&req);

            std::cout << "[Xapian-TA] Query from Linux: " << shm_buf << std::endl;

            // 4. Check if Add or Search
            try {
                std::string input(shm_buf);
                if (input.rfind("LOAD:", 0) == 0) {
                    std::string payload = input.substr(5);
                    if (!payload.empty()) {
                        std::string decrypted = xor_cipher(payload);
                        
                        db = Xapian::InMemory::open(); // clear db
                        secure_docs.clear();
                        
                        Xapian::TermGenerator tg;
                        tg.set_stemmer(Xapian::Stem("en"));
                        
                        std::istringstream stream(decrypted);
                        std::string line;
                        while (std::getline(stream, line)) {
                            if (!line.empty()) {
                                secure_docs.push_back(line);
                                Xapian::Document new_doc;
                                new_doc.set_data(line);
                                tg.set_document(new_doc);
                                tg.index_text(line);
                                db.add_document(new_doc);
                            }
                        }
                        db.commit();
                        std::cout << "[Xapian-TA] Loaded and decrypted " << secure_docs.size() << " documents from SSD." << std::endl;
                    }
                    strncpy(shm_buf, "LOAD_OK", SHM_SIZE - 1);
                } else if (input.rfind("ADD:", 0) == 0) {
                    std::string doc_content = input.substr(4);
                    size_t start = doc_content.find_first_not_of(" \t");
                    if (start != std::string::npos) doc_content = doc_content.substr(start);

                    secure_docs.push_back(doc_content);

                    Xapian::Document new_doc;
                    new_doc.set_data(doc_content);
                    
                    Xapian::TermGenerator tg;
                    tg.set_stemmer(Xapian::Stem("en"));
                    tg.set_document(new_doc);
                    tg.index_text(doc_content);
                    
                    db.add_document(new_doc);
                    db.commit();
                    
                    // Serialize & Encrypt for SSD
                    std::string serialized = "";
                    for (const auto& d : secure_docs) {
                        serialized += d + "\n";
                    }
                    std::string encrypted = xor_cipher(serialized);
                    std::string res = "SAVE:" + encrypted;
                    
                    strncpy(shm_buf, res.c_str(), SHM_SIZE - 1);
                    std::cout << "[Xapian-TA] Added document and sent encrypted blob to Linux." << std::endl;
                } else if (input.rfind("TEST_EXCEPTION", 0) == 0) {
                    std::cout << "[-TA] Testing ExcXapianeption..." << std::endl;
                    throw std::runtime_error("CATCH THÀNH CÔNG! Đây là lỗi cố ý để test try-catch trong TrustZone.");
                } else {
                    Xapian::QueryParser qp;
                    qp.set_stemmer(Xapian::Stem("en"));
                    qp.set_database(db);
                    qp.set_stemming_strategy(Xapian::QueryParser::STEM_SOME);

                    Xapian::Query query = qp.parse_query(shm_buf);
                    std::cout << "[Xapian-TA] Parsed query: " << query.get_description() << std::endl;

                    Xapian::Enquire enquire(db);
                    enquire.set_query(query);
                    Xapian::MSet matches = enquire.get_mset(0, 10);

                    std::string result = "Found " + std::to_string(matches.get_matches_estimated()) + " matches.\n";
                    for (Xapian::MSetIterator i = matches.begin(); i != matches.end(); ++i) {
                        result += "- " + i.get_document().get_data() + "\n";
                    }
                    
                    // Write result back to SHM
                    strncpy(shm_buf, result.c_str(), SHM_SIZE - 1);
                    std::cout << "[Xapian-TA] Search complete! Sending back result." << std::endl;
                }
            } catch (const Xapian::Error &e) {
                std::string err = "Xapian Exception: " + std::string(e.get_msg());
                strncpy(shm_buf, err.c_str(), SHM_SIZE - 1);
                std::cout << "[Xapian-TA] " << err << std::endl;
            } catch (const std::exception &e) {
                std::string err = "Standard Exception: " + std::string(e.what());
                strncpy(shm_buf, err.c_str(), SHM_SIZE - 1);
                std::cout << "[Xapian-TA] " << err << std::endl;
            }

            // 5. SMC back to Linux to signal completion
            req.x1 = 0; // SMC_EXIT_SHADOW maybe?
            req.x2 = 0;
            usys_tee_switch_req(&req);
        }
    } else {
        std::cout << "[Xapian-TA] Failed to map SHM!" << std::endl;
    }

    while (1) {
        // Keep alive
    }
    return 0;
}
