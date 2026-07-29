cmd_certs/built-in.a := echo >/dev/null; rm -f certs/built-in.a; aarch64-linux-gnu-ar cDPrST certs/built-in.a certs/system_keyring.o certs/system_certificates.o certs/common.o
