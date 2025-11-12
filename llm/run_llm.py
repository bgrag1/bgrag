import os
import sys
import torch
import random 
import json
# from vllm import LLM, SamplingParams
from transformers import AutoModelForCausalLM, AutoTokenizer, pipeline
from tqdm import tqdm
from prompts import *

model_name = "/home/m2024-djj/llm/model/DeepSeek-Coder-V2-Lite-Instruct"
# model_name = "/home/m2024-djj/llm/model/deepseek-ai/deepseek-coder-6.7b-instruct"
qwen_name = '/home/m2024-djj/llm/model/Qwen2.5-Coder-14B-Instruct'
codellama_name = '/home/m2024-djj/llm/model/CodeLlama-7b-Instruct'
phi_name = '/home/m2024-djj/llm/model/Phi-4-mini-instruct'
mistral_name = '/home/m2024-djj/llm/model/mistral-7b-instruct-v0.3'


class Model_phi:
    def __init__(self, model=phi_name):
        # self.device = "auto"
        # self.model = AutoModelForCausalLM.from_pretrained(
        #     model,
        #     torch_dtype="auto",
        #     trust_remote_code=True,
        #     device_map="auto",
        # )
        # self.tokenizer = AutoTokenizer.from_pretrained(model, trust_remote_code=True)
        # self.model = LLM(model=phi_name, trust_remote_code=True)
        self.device = "cuda"

    def generate(self, prompt):
        messages = [
    {"role": "system", "content": "You are a helpful AI assistant."},
    {"role": "user", "content": prompt},
]

        # sampling_params = SamplingParams(
        # max_tokens=512,
        # temperature=0.0,
        # )

        # output = self.model.chat(messages=messages, sampling_params=sampling_params)   
        # return output[0].outputs[0].text 

class Model_mistral:
    def __init__(self, model=phi_name):
        self.device = "cuda"
        self.model = AutoModelForCausalLM.from_pretrained(
            model,
            trust_remote_code=True,
        )
        self.tokenizer = AutoTokenizer.from_pretrained(model, trust_remote_code=True)

    def generate(self, prompt):
        enhanced_prompt = f"<s>[INST] {prompt} [/INST]"
        messages = [
    {"role": "user", "content": enhanced_prompt},
]

        encodeds = self.tokenizer.apply_chat_template(messages, return_tensors="pt")

        model_inputs = encodeds.to(self.device)
        self.model.to(self.device)

        generated_ids = self.model.generate(model_inputs, max_new_tokens=512, do_sample=True)
        decoded = self.tokenizer.batch_decode(generated_ids)  
        return decoded[0]

# class Model_Deepseek:
#     def __init__(self, model=model_name):
#         self.device = "auto"
#         # self.device = 'cuda:1'
#         self.model = AutoModelForCausalLM.from_pretrained(
#             model,
#             torch_dtype=torch.bfloat16,
#             trust_remote_code=True,
#             device_map="auto",
#         )
#         self.tokenizer = AutoTokenizer.from_pretrained(model, trust_remote_code=True)


#     def generate(self, prompt):
#         messages = [{"role": "user", "content": prompt}]
    
#         inputs = self.tokenizer.apply_chat_template(
#                 messages,
#                 add_generation_prompt=True,
#                 return_tensors="pt"
#             )
#         inputs = inputs.to(self.model.device)
#         print(f"Len prompt: {len(prompt)}")
#         outputs = self.model.generate( 
#                     inputs, max_new_tokens=512, 
#                     do_sample=False, 
#                     top_k=50, 
#                     top_p=0.95, 
#                     num_return_sequences=1, 
#                     eos_token_id=self.tokenizer.eos_token_id,
#             )

#         response = self.tokenizer.decode(outputs[0][len(inputs[0]):], skip_special_tokens=True)
#         return response

class Model_Deepseek:
    def __init__(self, model=model_name):
        self.device = "auto"
        # self.device = 'cuda:1'
        self.model = AutoModelForCausalLM.from_pretrained(
            model,
            torch_dtype=torch.bfloat16,
            trust_remote_code=True,
            device_map="auto",
        )
        self.tokenizer = AutoTokenizer.from_pretrained(model, trust_remote_code=True)
        self.history = []

    def generate(self, prompt):
        self.history.append({"role": "user", "content": prompt})
    
        inputs = self.tokenizer.apply_chat_template(
                self.history,
                add_generation_prompt=True,
                return_tensors="pt"
            )
        inputs = inputs.to(self.model.device)
        print(f"Len prompt: {len(prompt)}")
        outputs = self.model.generate( 
                    inputs, max_new_tokens=512, 
                    do_sample=False, 
                    top_k=50, 
                    top_p=0.95, 
                    num_return_sequences=1, 
                    eos_token_id=self.tokenizer.eos_token_id,
            )

        response = self.tokenizer.decode(outputs[0][len(inputs[0]):], skip_special_tokens=True)
        self.history.append({"role": "assistant", "content": response})
        return response
    
    def reset_history(self):
        """
        提供一个方法来清空历史记录，开始新的对话。
        """
        print("Conversation history has been reset.")
        self.history = []

class Model_Qwen:
    def __init__(self, model=qwen_name):
        self.device = 'cuda:1'
        self.model = AutoModelForCausalLM.from_pretrained(
            model,
            torch_dtype=torch.float16,
            trust_remote_code=True,
        ).to(self.device)
        self.tokenizer = AutoTokenizer.from_pretrained(model, trust_remote_code=True)


    def generate(self, prompt):
        messages = [
                {"role": "system", "content": "You are Qwen, created by Alibaba Cloud. You are a helpful assistant."},
                {"role": "user", "content": prompt}
        ]
    
        text = self.tokenizer.apply_chat_template(
                messages,
                tokenize=False,
                add_generation_prompt=True,
            )
        
        model_inputs = self.tokenizer([text], return_tensors="pt").to(self.device)

        generated_ids = self.model.generate( 
                    **model_inputs,
                    max_new_tokens=2048,
                    do_sample = False,
                    temperature = None,
                    top_p = None,
                    top_k = None,
            )

        generated_ids = [
            output_ids[len(input_ids):] for input_ids, output_ids in zip(model_inputs.input_ids, generated_ids)
        ]

        response = self.tokenizer.batch_decode(generated_ids, skip_special_tokens=True)[0]
        return response

class Model_codellama:
    def __init__(self, model=codellama_name):
        # 检查可用的GPU
        if torch.cuda.is_available():
            self.device = "cuda"
        else:
            self.device = "cpu"
            
        self.model = AutoModelForCausalLM.from_pretrained(
            model,
            torch_dtype=torch.float16,
            trust_remote_code=True,
            device_map="auto",
        )
        self.tokenizer = AutoTokenizer.from_pretrained(model, trust_remote_code=True)
        
        # CodeLlama 可能需要设置 pad_token
        if self.tokenizer.pad_token is None:
            self.tokenizer.pad_token = self.tokenizer.eos_token

    def generate(self, prompt):
        # CodeLlama 通常使用更简单的 prompt 格式，不需要复杂的 chat template
        # 直接使用输入的 prompt
        enhanced_prompt = f"<s>[INST] {prompt} [/INST]"
        # 编码输入
        inputs = self.tokenizer(
            enhanced_prompt,
            return_tensors="pt",
            truncation=True,
            max_length=4096,  # 根据模型的最大输入长度调整
        )
        inputs = inputs.to(self.model.device)
        
        print(f"Len prompt: {len(prompt)}")
        
        # 生成响应
        with torch.no_grad():
            outputs = self.model.generate(
                **inputs,
                max_new_tokens=512,
                do_sample=False,
                temperature=0.1,
                top_p=0.95,
                num_return_sequences=1,
                eos_token_id=self.tokenizer.eos_token_id,
                pad_token_id=self.tokenizer.pad_token_id,
            )

        # 解码响应（只获取新生成的部分）
        response = self.tokenizer.decode(
            outputs[0][len(inputs.input_ids[0]):], 
            skip_special_tokens=True
        )
        return response

def run_dataset(select, retrieve, dataset_path, dataset_name):
    # 得到模型
    if select == '1':
        model = Model_Deepseek()
        model_name = 'DeepSeek'
    elif select == '2':
        model = Model_Qwen()
        model_name = 'Qwen'
    elif select == '3':
        model = Model_codellama()
        model_name = 'CodeLlama'
    if not retrieve:
        func_path_novul = os.path.join(dataset_path, 'novul')
        func_path_vul = os.path.join(dataset_path, 'vul')
        ret_novul = {}
        ret_vul = {}

        for file_name in tqdm(os.listdir(func_path_novul), desc='Processing Non-Vulnerable Functions'):
            model.reset_history()
            explanation = ""
            prompt = generate_prompt_no_retrieve(dataset_name, 0, file_name)
            try:
                response = model.generate(prompt)
                if '\"is_vulnerable\": true' in response:
                    # 用于生成解释的prompt不包含对话历史
                    # model.reset_history()
                    explanation = model.generate(explanation_prompt(dataset_name, 0, file_name))
            except Exception as e:
                with open(os.path.join(dataset_path, 'result/novul_error.txt'), 'a') as f:
                    f.write(f'{model_name} -- {file_name} -- {len(prompt)} -- {e}\n')
                continue
            ret_novul[file_name] = {'len_prompt':len(prompt), 'ret': response, 'explanation': explanation}
        with open(os.path.join(dataset_path, 'result', f'{model_name}_novul_0_explanation_1.json'), 'w') as f:
            json.dump(ret_novul, f, ensure_ascii=False, indent=2)

        # 处理vul
        for file_name in tqdm(os.listdir(func_path_vul), desc='Processing Vulnerable Functions'):
            model.reset_history()
            explanation = ""
            prompt = generate_prompt_no_retrieve(dataset_name, 1, file_name)
            try:
                response = model.generate(prompt)
                if '\"is_vulnerable\": true' in response:
                    # 用于生成解释的prompt不包含对话历史
                    # model.reset_history()
                    explanation = model.generate(explanation_prompt(dataset_name, 1, file_name))
            except Exception as e:
                with open(os.path.join(dataset_path, 'result/vul_error.txt'), 'a') as f:
                    f.write(f'{model_name} -- {file_name} -- {len(prompt)} -- {e}\n')
                continue
            ret_vul[file_name] = {'len_prompt':len(prompt), 'ret': response, 'explanation': explanation}
        with open(os.path.join(dataset_path, 'result', f'{model_name}_vul_0_explanation_1.json'), 'w') as f:
            json.dump(ret_vul, f, ensure_ascii=False, indent=2)
    else:
        func_path_novul = os.path.join(dataset_path, 'novul')
        func_path_vul = os.path.join(dataset_path, 'vul')
        ret_novul = {}
        ret_vul = {}
        if_explanation = 0

        for file_name in tqdm(os.listdir(func_path_novul), desc='Processing Non-Vulnerable Functions'):
            model.reset_history()
            explanation = ""
            prompt = generate_prompt_new_only_func(dataset_name, 0, file_name)
            if prompt == False:
                print(f'未找到{file_name}的检索结果，跳过')
                continue
            if len(prompt) > 16384:
                continue
            try:
                response = model.generate(prompt)
                if 'is_vulnerable' in response:
                    print(f"Response: {response}")
                if '\"is_vulnerable\": true' in response and if_explanation == 1:
                    # 用于生成解释的prompt不包含对话历史
                    # model.reset_history()
                    explanation = model.generate(explanation_prompt(dataset_name, 0, file_name))
            except Exception as e:
                with open(os.path.join(dataset_path, 'result', 'novul_error_1.txt'), 'a') as f:
                    f.write(f'{model_name} -- {file_name} -- {len(prompt)} -- {e}\n')
                continue
            ret_novul[file_name] = {'len_prompt':len(prompt), 'ret': response, 'explanation': explanation}
        with open(os.path.join(dataset_path, 'result', f'{model_name}_novul_1_only_func_2.json'), 'w') as f:
            json.dump(ret_novul, f, ensure_ascii=False, indent=2)
 
        # 处理vul
        for file_name in tqdm(os.listdir(func_path_vul), desc='Processing Vulnerable Functions'):
            # model.reset_history()
            explanation = ""
            prompt = generate_prompt_new_only_func(dataset_name, 1, file_name)
            # with open(prompt_txt_path, 'a') as f:
            #     f.write(f'-----{file_name}-----\n{prompt}\n\n')
            if prompt == False:
                print(f'未找到{file_name}的检索结果，跳过')
                continue
            if len(prompt) > 16384:
                continue
            try:
                response = model.generate(prompt)
                if '\"is_vulnerable\": true' in response and if_explanation == 1:
                    # 用于生成解释的prompt不包含对话历史
                    # model.reset_history()
                    explanation = model.generate(explanation_prompt(dataset_name, 1, file_name))
            except Exception as e:
                with open(os.path.join(dataset_path, 'result', 'vul_error_1.txt'), 'a') as f:
                    f.write(f'{model_name} -- {file_name} -- {len(prompt)} -- {e}\n')
                continue
            ret_vul[file_name] = {'len_prompt':len(prompt), 'ret': response, 'explanation': explanation}
        with open(os.path.join(dataset_path, 'result', f'{model_name}_vul_1_only_func_2.json'), 'w') as f:
            json.dump(ret_vul, f, ensure_ascii=False, indent=2)

def main():
    select = input("Select model to run (1: DeepSeek, 2: Qwen, 3: CodeLlama, 4: phi, 5: mistral): ")
    if select not in ['1', '2', '3', '4', '5']:
        print("Invalid selection. Exiting.")
        return
    select2 = input("Use which dataset?(1. secvul, 2. cvefixes, 3. megavul): ")
    if select2 == '1':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/'
        dataset_name = 'secvul'
    elif select2 == '2':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes/'
        dataset_name = 'cvefixes'
    elif select2 == '3':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/'
        dataset_name = 'megavul'
    elif select2 == '123':
        run_dataset(select, True, '/home/m2024-djj/Vulrag/rag/test_data_secvul/', 'secvul')
        run_dataset(select, True, '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes/', 'cvefixes')
        run_dataset(select, True, '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul/', 'megavul')
    else:
        print("Invalid selection. Exiting.")
        return
    select3 = input("If retrieve?(y/n)")
    if select3.lower() == 'y':
        retrieve = True
    elif select3.lower() == 'n':
        retrieve = False
    else:
        print("Invalid selection. Exiting.")
        return
    
    run_dataset(select, retrieve, dataset_path, dataset_name)

if __name__ == '__main__':
    main()

