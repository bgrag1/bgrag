import pickle
import random
import os
import numpy as np
import torch
from sklearn.cluster import MiniBatchKMeans
from collections import defaultdict

# 聚类中心数
N_CLUSTERS_GT10 = 10
N_CLUSTERS_RANGE5_10 = 5

# 设置随机种子以确保可重复性
def derandom():
    seed = 202203
    random.seed(seed)
    os.environ['PYTHONHASHSEED'] = str(seed)
    np.random.seed(seed)
    torch.manual_seed(seed)
    torch.cuda.manual_seed(seed)
    torch.backends.cudnn.deterministic = True

# 训练 KMeans 模型并保存
def train_kmeans(n, inputs, output):
    kmeans = MiniBatchKMeans(n_clusters=n, random_state=0, verbose=1, max_iter=300, batch_size=3000)
    for i in range(0, len(inputs), 3000):
        kmeans.partial_fit(inputs[i:i+3000])
    with open(output, "wb") as ff:
        pickle.dump(kmeans, ff)
    return kmeans

# 生成边并统计每个簇的出现次数
def generate_edges(n, final_data, cluster_model):
    edges = []
    func_idx_begin = n
    cluster_count = defaultdict(int)  # 统计每个簇的出现次数

    for func in final_data:
        tmp_vec = [np.array(i) * 1000 for i in func["slices_vec"]]
        if len(tmp_vec) == 0:
            func_idx_begin += 1
            continue
        tmp_labs = cluster_model.predict(tmp_vec)
        for i in range(len(tmp_vec)):
            lab = tmp_labs[i]
            edges.append([func['file'], func_idx_begin, lab])
            cluster_count[lab] += 1
        func_idx_begin += 1

    return edges, cluster_count

# 保存边到文件
def save_edges(edges, fname):
    with open(fname, "w") as f:
        for edge in edges:
            f.write(f"{edge[0]} {edge[1]} {edge[2]}\n")
    print(f"Edges saved to {fname}.")

# 保存筛选后的切片向量和函数名
def save_filtered_slices_and_func_names(filtered_slices, func_names, slice_fname, func_name_fname):
    with open(slice_fname, "wb") as f:
        pickle.dump(filtered_slices, f)
    with open(func_name_fname, "w") as f:
        for name in func_names:
            f.write(f"{name}\n")
    print(f"Filtered slices saved to {slice_fname}.")
    print(f"Function names saved to {func_name_fname}.")

if __name__ == "__main__":
    # 设置随机种子
    derandom()
    
    # 设置数据集路径和聚类中心数
    home_path = '/home/m2024-djj/Vulrag/'
    slice_vec_dirs = {
        os.path.join(home_path, 'rag/slice_vec/gt10'): N_CLUSTERS_GT10,
        os.path.join(home_path, 'rag/slice_vec/range5-10'): N_CLUSTERS_RANGE5_10
    }
    graph_base_path = os.path.join(home_path, 'rag/graph')

    # 遍历每个切片向量目录
    for slice_vec_dir, n_clusters in slice_vec_dirs.items():
        # 遍历目录中的每个 CWE 文件
        for cwe_file in os.listdir(slice_vec_dir):
            if not cwe_file.endswith('.pkl'):
                continue
            cwe_name = cwe_file.replace('.pkl', '')
            cwe_graph_path = os.path.join(graph_base_path, cwe_name)
            # 确保输出目录存在
            if not os.path.exists(cwe_graph_path):
                os.makedirs(cwe_graph_path, exist_ok=True)

            # 加载数据集
            dataset_path = os.path.join(slice_vec_dir, cwe_file)
            dataset = pickle.load(open(dataset_path, "rb"))

            # 提取 before 数据
            dataset_before = []
            for func in dataset:
                if not func['file'].endswith('_after'):
                    dataset_before.append(func)

            # 提取切片向量作为输入数据
            inputs = []
            for func in dataset_before:
                for slice_vec in func["slices_vec"]:
                    inputs.append(np.array(slice_vec) * 1000)

            # 动态设置聚类中心数
            effective_n_clusters = n_clusters
            if len(inputs) < effective_n_clusters:
                effective_n_clusters = len(inputs)

            # 训练聚类模型
            kmeans_path = os.path.join(cwe_graph_path, f"{cwe_name}_kmeans.pkl")
            kmeans = train_kmeans(effective_n_clusters, inputs, kmeans_path)

            # 生成边并统计每个簇的出现次数
            edges, cluster_count = generate_edges(effective_n_clusters, dataset_before, kmeans)
            edges_path = os.path.join(cwe_graph_path, f"{cwe_name}.edges")
            save_edges(edges, edges_path)

            # 根据目录选择出现次数最多的簇的数量
            num_top_clusters = 3 if slice_vec_dir.endswith('gt10') else 1
            top_clusters = sorted(cluster_count.items(), key=lambda x: x[1], reverse=True)[:num_top_clusters]
            top_cluster_labels = [item[0] for item in top_clusters]

            print(f"Top {num_top_clusters} clusters: {top_cluster_labels}")

            # 筛选切片向量和保存对应的函数名
            filtered_slices = []
            func_names = []
            for func in dataset_before:
                for slice_vec in func["slices_vec"]:
                    tmp_vec = np.array(slice_vec) * 1000
                    lab = kmeans.predict([tmp_vec])[0]
                    if lab in top_cluster_labels:
                        filtered_slices.append(tmp_vec)
                        func_names.append(func['file'])
            slice_fname = os.path.join(cwe_graph_path, f"{cwe_name}_filtered_slices.pkl")
            func_name_fname = os.path.join(cwe_graph_path, f"{cwe_name}_func_name.txt")
            save_filtered_slices_and_func_names(filtered_slices, func_names, slice_fname, func_name_fname)

    print("All done!")