#include <xapian.h>
#include <iostream>
#include <string>
#include <cstring>

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

    Xapian::Document doc1;
    doc1.set_data("This is a secret document stored in TrustZone.");
    termgenerator.set_document(doc1);
    termgenerator.index_text("This is a secret document stored in TrustZone.");
    db.add_document(doc1);

    Xapian::Document doc2;
    doc2.set_data("Another top secret AI model data.");
    termgenerator.set_document(doc2);
    termgenerator.index_text("Another top secret AI model data.");
    db.add_document(doc2);
    
    // NEW FEATURE: Added Version 2.0 Document
    Xapian::Document doc3;
    doc3.set_data("Welcome to Xapian-TA Version 2.0! The infinite loop bug is fixed.");
    termgenerator.set_document(doc3);
    termgenerator.index_text("Welcome to Xapian-TA Version 2.0! The infinite loop bug is fixed.");
    db.add_document(doc3);

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
                if (input.rfind("ADD:", 0) == 0) {
                    std::string doc_content = input.substr(4);
                    size_t start = doc_content.find_first_not_of(" \t");
                    if (start != std::string::npos) doc_content = doc_content.substr(start);

                    Xapian::Document new_doc;
                    new_doc.set_data(doc_content);
                    
                    Xapian::TermGenerator tg;
                    tg.set_stemmer(Xapian::Stem("en"));
                    tg.set_document(new_doc);
                    tg.index_text(doc_content);
                    
                    db.add_document(new_doc);
                    db.commit();
                    
                    std::string res = "Successfully added to TEE InMemory Database!\nContent: " + doc_content;
                    strncpy(shm_buf, res.c_str(), SHM_SIZE - 1);
                    std::cout << "[Xapian-TA] Added document to InMemory DB." << std::endl;
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
