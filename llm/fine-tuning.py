from transformers import AutoTokenizer
from sentence_transformers import SentenceTransformer, losses
from sentence_transformers.losses import MultipleNegativesRankingLoss
from sentence_transformers.readers import InputExample
from sentence_transformers.evaluation import EmbeddingSimilarityEvaluator
from torch.utils.data import DataLoader
from datasets import Dataset
from sklearn.model_selection import train_test_split
import os
os.environ["TOKENIZERS_PARALLELISM"] = "false"
from tqdm import tqdm
import random
import json
import torch
import numpy as np

# random.seed(42)
# np.random.seed(42)
# torch.manual_seed(42)
# if torch.cuda.is_available():
#     torch.cuda.manual_seed_all(42)

# 1. 定义模型名称
# model_name = '/home/m2024-djj/llm/model/codebert-base' 
model_name = '/home/m2024-djj/llm/model/fine-tuned-codebert-vulnerability-30'  # 使用微调后的模型路径

# 2. 加载预训练的CodeBERT模型
# SentenceTransformer会自动处理模型的池化（pooling），通常使用[CLS] token的平均池化
device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
model = SentenceTransformer(model_name).to(device)
tokenizer = AutoTokenizer.from_pretrained(model_name)

# 3. 准备数据
func_text_path = '/home/m2024-djj/Vulrag/rag/func_text'
cwe_n_path = '/home/m2024-djj/Vulrag/rag/get_cwe_description/cwe_description_drop.json'
with open (cwe_n_path, 'r') as f:
    cwe_description = json.load(f)
cwe_n = {}
for cwe in cwe_description.keys():
    cwe_n[cwe] = cwe_description[cwe]['description']

raw_data = []
max_samples_per_cwe = 1500
for cwe in tqdm(os.listdir(func_text_path), desc="Loading CWE data"):
    cwe_data = []
    if cwe not in cwe_n.keys():
        continue
    cwe_path = os.path.join(func_text_path, cwe)
    for repo in os.listdir(cwe_path):
        if repo.endswith('.txt'):
            continue
        file_path = os.path.join(cwe_path, repo)
        for file in os.listdir(file_path):
            if file.endswith('after.c'):
                continue
            with open(os.path.join(file_path, file), 'r', encoding='utf-8') as f:
                code = f.read()
            cwe_data.append({
                'code': code,
                'desc': cwe_n[cwe]
            })
    if len(cwe_data) > max_samples_per_cwe:
        selected_samples = random.sample(cwe_data, max_samples_per_cwe)
    else:
        selected_samples = cwe_data
    raw_data.extend(selected_samples)
print(f"总共收集了 {len(raw_data)} 个平衡后的训练样本")

print("开始对长代码进行分块处理...")
train_examples = []
max_seq_length = 512 # CodeBERT的默认最大长度
stride = max_seq_length // 4 # 重叠步长，例如1/4，可以根据需要调整

for item in tqdm(raw_data, desc="Processing code chunks"):
    long_code = item['code']
    description = item['desc']

    # 对长代码进行分块
    # encode_plus 返回一个字典，包含 input_ids, attention_mask 等
    # truncation=False 确保不会在这一步截断
    # return_overflowing_tokens=True 会返回所有生成的块的token ID
    # stride 控制重叠
    encoded_segments = tokenizer(
        long_code,
        add_special_tokens=True, # 确保包含 [CLS] 和 [SEP]
        max_length=max_seq_length,
        # padding='max_length', # Pad to max_length for consistent input shape if needed
        truncation=True, # 这一步还是需要truncation，但我们通过next_batch来实现多块
        return_attention_mask=True,
        return_token_type_ids=False,
        return_overflowing_tokens=True,
        stride=stride,
        # return_tensors='pt'
    )

    for i in range(len(encoded_segments['input_ids'])):
        chunk_input_ids = encoded_segments['input_ids'][i]
        
        # 解码回文本，跳过特殊token
        chunk_text = tokenizer.decode(chunk_input_ids, skip_special_tokens=True)
        
        # 改进5: 确保 chunk_text 非空且有意义
        if len(chunk_text.strip()) > 50: # 避免空字符串或只有空格的块
            train_examples.append(InputExample(texts=[chunk_text, description], label=1.0))

print(f"Total number of training examples after chunking: {len(train_examples)}")

# 划分训练集和验证集
train_examples, val_examples = train_test_split(
    train_examples, test_size=0.1, 
    # random_state=42
    )
print(f"训练集: {len(train_examples)} 样本, 验证集: {len(val_examples)} 样本")

# 创建DataLoader
train_dataloader = DataLoader(train_examples, shuffle=True, batch_size=64)

# 准备验证集
val_sentences1 = [example.texts[0] for example in val_examples]
val_sentences2 = [example.texts[1] for example in val_examples]
val_scores = [example.label for example in val_examples]

evaluator = EmbeddingSimilarityEvaluator(
    val_sentences1, val_sentences2, val_scores,
    batch_size=32, show_progress_bar=True, name="dev"
)

# 4. 定义损失函数
# MultipleNegativeRankingLoss 是对比学习的常用损失函数，它会利用批次内的其他样本作为负样本。
# 适合于 (query, positive_passage) 对。
train_loss = MultipleNegativesRankingLoss(model=model)

# 5. 定义训练参数
num_epochs = 40
evaluation_steps = 1000 # 每1000步进行一次验证
warmup_steps = int(len(train_dataloader) * num_epochs * 0.1) # 10% of total training steps

output_path = "/home/m2024-djj/llm/model/fine-tuned-codebert-vulnerability-31"
os.makedirs(output_path, exist_ok=True)


# 6. 开始微调
print("Starting fine-tuning...")
model.fit(train_objectives=[(train_dataloader, train_loss)],
          evaluator=evaluator,
          epochs=num_epochs,
          warmup_steps=warmup_steps,
          evaluation_steps=evaluation_steps,
          output_path=output_path,
          save_best_model=True,
        #   optimizer_params={'lr': 2e-5},
          optimizer_params={'lr': 3e-5}, 
          show_progress_bar=True
          )

print(f"Fine-tuning complete. Model saved to {output_path}")