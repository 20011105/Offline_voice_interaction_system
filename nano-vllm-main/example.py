import os
from nanovllm import LLM, SamplingParams
from transformers import AutoTokenizer


def main():
    path = os.path.expanduser("C:\\vllm_nano\\nano-vllm-main\\Qwen3-0.6B\\")
    tokenizer = AutoTokenizer.from_pretrained(path)
    llm = LLM(path, enforce_eager=True, tensor_parallel_size=0) # tensor_parallel_size=0表示不使用tensor parallelism分布式

    sampling_params = SamplingParams(temperature=0.6, max_tokens=256)

    print("===== 交互式对话模式 =====")
    print("提示：输入你的问题，按回车生成回答；按 Ctrl+C 退出程序\n")

    try:
        while True:

            
            # 获取用户输入
            prompt = input("你: ")
            if not prompt.strip():  # 忽略空输入
                continue

            # 格式化提示词（符合Qwen3的对话格式）
            formatted_prompt = tokenizer.apply_chat_template(
                [{"role": "user", "content": prompt}],
                tokenize=False,
                add_generation_prompt=True,
                enable_thinking=False  # 关闭思考过程，仅输出最终回答
            )

            # 调用模型生成回答
            outputs = llm.generate([formatted_prompt], sampling_params)
            response = outputs[0]["text"]

            # 打印结果
            print(f"\n模型: {response}\n")

    except KeyboardInterrupt:
        # 捕获Ctrl+C，优雅退出
        print("\n程序已退出，再见！")


    # prompts = [
    #     "introduce yourself",
    #     "list all prime numbers within 100",
    # ]
    # prompts = [
    #     tokenizer.apply_chat_template(
    #         [{"role": "user", "content": prompt}],
    #         tokenize=False,
    #         add_generation_prompt=True,
    #         enable_thinking=True
    #     )
    #     for prompt in prompts
    # ]
    # outputs = llm.generate(prompts, sampling_params)

    # for prompt, output in zip(prompts, outputs):
    #     print("\n")
    #     print(f"Prompt: {prompt!r}")
    #     print(f"Completion: {output['text']!r}")


if __name__ == "__main__":
    main()
