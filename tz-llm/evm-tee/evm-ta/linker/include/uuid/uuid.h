#ifndef UUID_H_MOCK_TEE
#define UUID_H_MOCK_TEE

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char uuid_t[16];

void uuid_generate_random(uuid_t out);
void uuid_unparse_lower(const uuid_t uu, char *out);

#ifdef __cplusplus
}
#endif

#endif // UUID_H_MOCK_TEE
