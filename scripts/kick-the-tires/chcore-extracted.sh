set -e

cd base/tee/tee_os_kernel/build
rm -rf ../oh_tee
cp -r /home/vectorxj/oh_tee ../
# The pristine oh_tee from the image carries no *.gguf at all, so chanmgr's
# llama-cli has no model to open. Not using prepare-model.sh: it copies from
# /home/vectorxj/chcore/opentrustee_llm/models/, which does not exist in this
# image. The meta ggufs actually live in oh_tee_bak/apps.
cp ../oh_tee_bak/apps/tinyllama-1.1b-chat-v1.0.Q8_0-meta.gguf ../oh_tee/apps/
ls -l ../oh_tee/apps/*.gguf
# BUG FIX: the rm -rf/cp -r above restores oh_tee from the pristine image
# copy on every single chcore.sh run, silently discarding any llama-cli/
# libllama.so/libggml.so freshly built via the separate llama-builder
# pipeline (build-llama.sh/build-llama-docker.sh's chcore_upload(), which
# writes into this same oh_tee/apps -- but BEFORE this restore step wipes
# it). Re-copy from the bind-mounted llama.cpp build-chcore output (source
# of truth for any TA-side llama.cpp/ggml source edit) so those changes
# actually reach the built TEE-OS image instead of silently reverting to
# whatever the docker image shipped with.
LLAMA_CHCORE_BUILD=/home/vectorxj/chcore/opentrustee_llm/llama.cpp/build-chcore
if [ -f "$LLAMA_CHCORE_BUILD/bin/llama-cli" ]; then
    cp "$LLAMA_CHCORE_BUILD/bin/llama-cli" ../oh_tee/apps/
    cp "$LLAMA_CHCORE_BUILD/src/libllama.so" ../oh_tee/apps/
    cp "$LLAMA_CHCORE_BUILD/ggml/src/libggml.so" ../oh_tee/apps/
    echo "chcore-extracted.sh: re-applied freshly-built llama-cli/libllama.so/libggml.so over the pristine oh_tee restore"
fi
./build_tee.sh
cd -
./device/board/opc/opi5plus/uboot/fast_build_uboot.sh /home/vectorxj/openharmony/out/uboot/src_tmp /home/vectorxj/openharmony/out/opi5plus/packages/phone/images /home/vectorxj/openharmony/ /home/vectorxj/openharmony/device/board/opc/opi5plus

