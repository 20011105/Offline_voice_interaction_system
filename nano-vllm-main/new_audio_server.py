# llm_server.py (最终版，与Linux的PUB/SUB架构匹配)
import os
import zmq
import re
import time
from nanovllm import LLM, SamplingParams
from transformers import AutoTokenizer

def split_text_into_chunks(text: str):
    """
    将长文本切分成更自然的短句块，以便流式TTS播放。
    """
    # 首先清理模型可能输出的特殊结束标记
    text = text.strip().replace("<|im_end|>", "")
    if not text:
        return []
    
    # 使用正则表达式按常见标点符号进行分割，保留标点
    text = text.replace('…', '...').replace('...', '。')
    sentences = re.split(r'([,.;?!，。；？！\n])', text)
    
    # 将句子和其后的标点符号合并成一个块
    chunks = []
    for i in range(0, len(sentences) - 1, 2):
        chunk = sentences[i] + sentences[i+1]
        if chunk.strip():
            chunks.append(chunk.strip())
    # 添加最后一个可能的句子片段
    if len(sentences) % 2 == 1 and sentences[-1].strip():
        chunks.append(sentences[-1].strip())
        
    return chunks

def main():
    # --- 1. 初始化LLM ---
    path = os.path.expanduser("C:\\vllm_nano\\nano-vllm-main\\Qwen3-0.6B\\")
    tokenizer = AutoTokenizer.from_pretrained(path)
    llm = LLM(path, enforce_eager=True, tensor_parallel_size=0)
    sampling_params = SamplingParams(temperature=0.6, max_tokens=256)
    print("LLM模型加载完成！")

    # --- 2. 设置ZMQ ---
    context = zmq.Context()
    
    # a. 作为服务端，接收来自 a_s_r 的请求 (端口 6666)
    asr_socket = context.socket(zmq.REP)
    asr_socket.bind("tcp://*:6666")

    # b. 作为客户端，连接到 Linux TTS 服务的数据端口 (7777)
    # !!! 重要：请将 "LINUX_VM_IP" 替换为您的Linux虚拟机的IP地址 !!!
    linux_vm_ip = "192.168.118.128" # <--- 在这里修改IP地址
    tts_socket = context.socket(zmq.REQ)
    tts_socket.connect(f"tcp://{linux_vm_ip}:7777")
    
    print(f"\n===== LLM服务已启动 (最终架构) =====")
    print(f"  -> 正在监听端口 6666 (用于接收ASR请求)")
    print(f"  -> 准备连接到TTS服务 at tcp://{linux_vm_ip}:7777")

    try:
        while True:
            # --- 3. 接收ASR请求 ---
            print("\n等待来自ASR的文本...")
            received_text = asr_socket.recv_string()
            print(f"[收到ASR] 文本: '{received_text}'")

            # --- 4. 调用LLM生成完整回答 ---
            formatted_prompt = tokenizer.apply_chat_template(
                [{"role": "user", "content": received_text}],
                tokenize=False, add_generation_prompt=True, enable_thinking=False
            )
            outputs = llm.generate([formatted_prompt], sampling_params)
            response_text = outputs[0]['text']
            
            # --- 5. 将回答分块并通过ZMQ流式发送给TTS服务 ---
            chunks = split_text_into_chunks(response_text)
            print(f"[LLM生成] 清理并分块后的回复: {chunks}")
            
            if not chunks:
                asr_socket.send_string("LLM无有效回复。")
                continue

            try:
                # 循环发送所有文本块到数据端口 7777
                for i, chunk in enumerate(chunks):
                    print(f"[发送TTS] 发送块 {i+1}/{len(chunks)}: '{chunk}'...")
                    tts_socket.send_string(chunk)
                    # 等待TTS的简单确认("OK")，然后才能发送下一条
                    reply = tts_socket.recv_string() 
                
                # 全部发送成功后，立即回复给ASR客户端
                print("[完成] 所有文本块已成功发送至TTS。")
                asr_socket.send_string("回答已发送至TTS进行播放。")

            except zmq.error.ZMQError as e:
                print(f"[错误] 与TTS服务通信失败: {e}")
                asr_socket.send_string(f"错误：与TTS服务通信失败。")

    except KeyboardInterrupt:
        print("\nLLM服务已关闭。")
    finally:
        asr_socket.close()
        tts_socket.close()
        context.term()

if __name__ == "__main__":
    main()


# # llm_server.py (流式TTS客户端版本)
# import os
# import zmq
# import time
# import re
# from nanovllm import LLM, SamplingParams
# from transformers import AutoTokenizer

# # 文本分块函数，让语音听起来更自然
# def split_text_into_chunks(text: str, max_len: int = 20):
#     """
#     将长文本切分成带有标点的短句块。
#     """
#     # 使用正则表达式按标点符号分割
#     text = text.strip()
#     sentences = re.split(r'([,./;?"!，。/；？“‘！\n])', text)
    
#     # 将句子和其后的标点合并
#     chunks = []
#     for i in range(0, len(sentences) - 1, 2):
#         chunks.append(sentences[i] + sentences[i+1])
#     if len(sentences) % 2 == 1 and sentences[-1]:
#         chunks.append(sentences[-1])

#     # 如果有超长的块，再进行硬切分
#     final_chunks = []
#     for chunk in chunks:
#         if len(chunk) > max_len:
#             for i in range(0, len(chunk), max_len):
#                 final_chunks.append(chunk[i:i+max_len])
#         elif chunk.strip():
#             final_chunks.append(chunk)
            
#     return final_chunks

# def main():
#     # --- 1. 初始化LLM和分词器 ---
#     path = os.path.expanduser("C:\\vllm_nano\\nano-vllm-main\\Qwen3-0.6B\\")
#     tokenizer = AutoTokenizer.from_pretrained(path)
#     llm = LLM(path, enforce_eager=True, tensor_parallel_size=0)
#     sampling_params = SamplingParams(temperature=0.6, max_tokens=256)
#     print("LLM模型加载完成！")

#     # --- 2. 设置ZMQ ---
#     context = zmq.Context()
    
#     # a. 作为服务端，接收来自 a_s_r 的请求
#     asr_socket = context.socket(zmq.REP)
#     asr_socket.bind("tcp://*:6666")

#     # b. 作为客户端，连接到 Linux TTS 服务
#     # !!! 重要：请将 "LINUX_VM_IP" 替换为您的Linux虚拟机的IP地址 !!!
#     linux_vm_ip = "192.168.118.128" # <--- 在这里修改IP地址
#     tts_data_socket = context.socket(zmq.REQ)
#     tts_data_socket.connect(f"tcp://{linux_vm_ip}:7777")

#     print(f"\n===== LLM服务已启动 (PUB/SUB 架构) =====")
    
#     # tts_status_socket = context.socket(zmq.REQ)
#     # tts_status_socket.connect(f"tcp://{linux_vm_ip}:6677")
    
#     print(f"\n===== LLM服务已启动 =====")
#     # print(f"  -> 正在监听端口 6666 (来自 ASR)")
#     print(f"  -> 准备连接到 TTS 服务 at tcp://{linux_vm_ip}:7777 和 6677")

#     try:
#         while True:
#             # --- 3. 接收ASR请求 ---
#             print("\n等待来自ASR的文本...")
#             received_text = asr_socket.recv_string()
#             print(f"[收到ASR] 文本: '{received_text}'")

#             # --- 4. 调用LLM生成完整回答 ---
#             formatted_prompt = tokenizer.apply_chat_template(
#                 [{"role": "user", "content": received_text}],
#                 tokenize=False, add_generation_prompt=True, enable_thinking=False
#             )
#             outputs = llm.generate([formatted_prompt], sampling_params)
#             response_text = outputs[0]['text']
#             response_text = response_text.replace("<|im_end|>", "").strip()
#             print(f"[LLM生成] 完整回答: '{response_text}'")

#             # --- 5. 将回答分块并发送给TTS服务 ---
#             chunks = split_text_into_chunks(response_text)
#             if not chunks:
#                 # 如果没有有效文本块，直接回复ASR
#                 asr_socket.send_string("LLM没有生成有效回复。")
#                 continue

#             try:
#                 # a. 通过状态端口通知TTS开始
#                 print("[发送TTS状态] 发送开始信号到 6677...")
#                 tts_status_socket.send_string("[llm -> tts] start play")
#                 status_reply = tts_status_socket.recv_string()
#                 print(f"[收到TTS状态] 回复: '{status_reply}'")

#                 # b. 循环发送所有文本块到数据端口
#                 for i, chunk in enumerate(chunks):
#                     message_to_send = chunk
#                     is_last_chunk = (i == len(chunks) - 1)
                    
#                     if is_last_chunk:
#                         message_to_send += "END" # 添加结束标志
                    
#                     print(f"[发送TTS数据] 发送块 {i+1}/{len(chunks)}: '{message_to_send}' 到 7777...")
#                     tts_data_socket.send_string(message_to_send)
#                     data_reply = tts_data_socket.recv_string()
#                     print(f"[收到TTS数据] 回复: '{data_reply}'")
#                     time.sleep(0.05) # 短暂延时，模拟真实说话间隔

#                 # c. 等待TTS播放完毕的信号
#                 print("[等待TTS状态] 等待播放完毕信号从 6677...")
#                 final_status = tts_status_socket.recv_string()
#                 print(f"[收到TTS状态] 最终状态: '{final_status}'")
                
#                 # d. 全部成功后，回复给ASR客户端
#                 asr_socket.send_string("回答已发送至TTS并播放完毕。")

#             except zmq.error.ZMQError as e:
#                 print(f"[错误] 与TTS服务通信失败: {e}")
#                 asr_socket.send_string("错误：与TTS服务通信失败。")


#     except KeyboardInterrupt:
#         print("\nLLM服务已关闭。")
#     finally:
#         asr_socket.close()
#         tts_data_socket.close()
#         tts_status_socket.close()
#         context.term()

# if __name__ == "__main__":
#     main()