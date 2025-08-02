# llm_server.py
import os
import zmq
from nanovllm import LLM, SamplingParams
from transformers import AutoTokenizer

def main():
    # --- 1. 初始化LLM和分词器 (这部分代码来自您的example.py) ---
    # !!! 请务必修改为您的模型路径 !!!
    path = os.path.expanduser("C:\\vllm_nano\\nano-vllm-main\\Qwen3-0.6B\\")
    
    print("正在加载分词器 (Tokenizer)...")
    tokenizer = AutoTokenizer.from_pretrained(path)
    
    print("正在加载LLM模型 (这可能需要一些时间)...")
    llm = LLM(path, enforce_eager=True, tensor_parallel_size=0)
    
    sampling_params = SamplingParams(temperature=0.6, max_tokens=256)
    print("LLM模型加载完成！")

    # --- 2. 设置ZMQ服务端 ---
    context = zmq.Context()
    # 创建一个REP (Reply)类型的socket
    socket = context.socket(zmq.REP)
    # 绑定到6666端口，"*"表示监听所有可用的网络接口
    socket.bind("tcp://*:6666")
    
    print("\n===== LLM服务已启动，正在监听端口 6666 =====")
    print("等待来自Linux客户端的文本...")

    try:
        while True:
            # --- 3. 循环等待并处理请求 ---
            
            # a. 阻塞等待，直到收到一个请求
            # 我们假设收发的是UTF-8编码的字符串
            received_text = socket.recv_string()
            print(f"\n[收到] 来自Linux的文本: '{received_text}'")

            # b. 格式化提示词并调用LLM (与您的example.py逻辑相同)
            formatted_prompt = tokenizer.apply_chat_template(
                [{"role": "user", "content": received_text}],
                tokenize=False,
                add_generation_prompt=True,
                enable_thinking=False
            )

            outputs = llm.generate([formatted_prompt], sampling_params)
            response_text = outputs[0]["text"]
            print(f"[生成] LLM的回答: '{response_text}'")

            # c. 将LLM的回答发送回客户端
            socket.send_string(response_text)

    except KeyboardInterrupt:
        print("\nLLM服务已关闭。")
    finally:
        socket.close()
        context.term()

if __name__ == "__main__":
    main()