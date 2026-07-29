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
./build_tee.sh
cd -
./device/board/opc/opi5plus/uboot/fast_build_uboot.sh /home/vectorxj/openharmony/out/uboot/src_tmp /home/vectorxj/openharmony/out/opi5plus/packages/phone/images /home/vectorxj/openharmony/ /home/vectorxj/openharmony/device/board/opc/opi5plus

