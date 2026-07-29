rm ./oh_tee/apps/*.gguf

model=$1

model_path=""
if [ "$model" = "tinyllama" ]; then
    model_path="tinyllama-1.1b-chat-v1.0.Q8_0"
elif [ "$model" = "gemma" ]; then
    model_path="gemma-2-2b-it-Q8_0"
elif [ "$model" = "qwen" ]; then
    model_path="qwen2.5-3b-instruct-q8_0"
elif [ "$model" = "phi" ]; then
    model_path="Phi-3-mini-4k-instruct.Q8_0"
elif [ "$model" = "llama" ]; then
    model_path="Meta-Llama-3-8B-Instruct.Q8_0"
else
    echo "invalid model $model"
    exit 1
fi

cp /home/vectorxj/chcore/opentrustee_llm/models/$model_path-meta.gguf ./oh_tee/apps

model=$2

model_path=""
if [ "$model" = "tinyllama" ]; then
    model_path="tinyllama-1.1b-chat-v1.0.Q8_0"
elif [ "$model" = "gemma" ]; then
    model_path="gemma-2-2b-it-Q8_0"
elif [ "$model" = "qwen" ]; then
    model_path="qwen2.5-3b-instruct-q8_0"
elif [ "$model" = "phi" ]; then
    model_path="Phi-3-mini-4k-instruct.Q8_0"
elif [ "$model" = "llama" ]; then
    model_path="Meta-Llama-3-8B-Instruct.Q8_0"
else
    echo "invalid model $model"
    exit 1
fi

cp /home/vectorxj/chcore/opentrustee_llm/models/$model_path-meta.gguf ./oh_tee/apps

model=$3

model_path=""
if [ "$model" = "tinyllama" ]; then
    model_path="tinyllama-1.1b-chat-v1.0.Q8_0"
elif [ "$model" = "gemma" ]; then
    model_path="gemma-2-2b-it-Q8_0"
elif [ "$model" = "qwen" ]; then
    model_path="qwen2.5-3b-instruct-q8_0"
elif [ "$model" = "phi" ]; then
    model_path="Phi-3-mini-4k-instruct.Q8_0"
elif [ "$model" = "llama" ]; then
    model_path="Meta-Llama-3-8B-Instruct.Q8_0"
else
    echo "invalid model $model"
    exit 1
fi

cp /home/vectorxj/chcore/opentrustee_llm/models/$model_path-meta.gguf ./oh_tee/apps
# Always also keep tinyllama's meta baked in (small model, fits easily
# within the 3GB TZASC pool), independent of whatever the caller passed
# for $1/$2/$3 -- used to test the pipeline decoupled from the CMA
# capacity limit documented in memory (model bigger than TZASC pool).
cp /home/vectorxj/chcore/opentrustee_llm/models/tinyllama-1.1b-chat-v1.0.Q8_0-meta.gguf ./oh_tee/apps
