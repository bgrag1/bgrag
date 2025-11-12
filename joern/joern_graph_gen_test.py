import os, sys
import glob
import argparse
from tqdm import tqdm
from multiprocessing import Pool
from functools import partial
import subprocess


# 获取path路径下所有文件，文件名保存在file_list
def get_all_file(path):
    path = path[0]
    file_list = []
    path_list = os.listdir(path)
    for path_tmp in path_list:
        full = path + path_tmp + '/'
        for file in os.listdir(full):
            file_list.append(file)
    return file_list

def parse_options():
    parser = argparse.ArgumentParser(description='Extracting Cpgs.')
    parser.add_argument('-i ', '--input', help='The dir path of input', type=str, default='/home/m2024-djj/Vulrag/rag/func_text')
    parser.add_argument('-o ', '--output', help='The dir path of output', type=str, default='/home/m2024-djj/Vulrag/rag/output_bin')
    parser.add_argument('-t ', '--type', help='The type of procedures: parse or export', type=str, default='export')
    parser.add_argument('-r ', '--repr', help='The type of representation: pdg or lineinfo_json', type=str, default='lineinfo_json')
    args = parser.parse_args()
    return args

def joern_parse(file, outdir):
    # 打开record_txt，存在rec_list
    record_txt =  os.path.join(outdir,"parse_res.txt")
    if not os.path.exists(record_txt):
        os.system("touch "+ record_txt)
    with open(record_txt,'r') as f:
        rec_list = f.readlines()
    # name.txt中的 name， 存在rec_list就返回
    name = file.split('/')[-1].split('.')[0]
    if name+'\n' in rec_list:
        print(" ====> has been processed: ", name)
        return
    print(' ----> now processing: ',name)
    # 输出文件夹 + name.bin，如果有了返回
    out = outdir + name + '.bin'
    if os.path.exists(out):
        return
    # 设置环境变量file、out，解析源文件，保存在out中，把文件名记录在record_txt
    os.environ['file'] = str(file)
    os.environ['out'] = str(out) #parse后的文件名与source文件名称一致
    os.system('sh joern-parse $file --language c --out $out')
    with open(record_txt, 'a+') as f:
        f.writelines(name+'\n')

def joern_parse_new(cwe, file, outdir):
    outdir = os.path.join(outdir, cwe)
    if not os.path.exists(outdir):
        os.makedirs(outdir)
    # 打开record_txt，存在rec_list
    record_txt =  os.path.join(outdir,"parse_res.txt")
    if not os.path.exists(record_txt):
        os.system("touch "+record_txt)
    with open(record_txt,'r') as f:
        rec_list = f.readlines()

    repo_name = file.split('/')[-2]
    func_name = file.split('/')[-1].split('.')[0]
    repo_path = os.path.join(outdir, repo_name)
    if not os.path.exists(repo_path):
        os.makedirs(repo_path)


    name = repo_name + '/' + func_name
    if name + '\n' in rec_list:
        print(" ====> has been processed: ", name)
        return
    print(' ----> now processing: ', name)
    # 输出文件夹 + name.bin，如果有了返回
    func_name = func_name + '.bin'
    out = os.path.join(repo_path, func_name)
    if os.path.exists(out):
        return

    # 设置环境变量file、out，解析源文件，保存在out中，把文件名记录在record_txt
    os.environ['file'] = str(file)
    os.environ['out'] = str(out) #parse后的文件名与source文件名称一致
    os.system('sh joern-parse $file --language c --out $out')
    with open(record_txt, 'a+') as f:
        f.writelines(name+'\n')

def joern_export(bin, outdir, repr):
    record_txt =  os.path.join(outdir,"export_res.txt")
    if not os.path.exists(record_txt):
        os.system("touch "+record_txt)
    with open(record_txt,'r') as f:
        rec_list = f.readlines()

    name = bin.split('/')[-1].replace(".bin","")
    out = os.path.join(outdir, name)
    if name+'\n' in rec_list:
        print(" ====> has been processed: ", name)
        return
    print(' ----> now processing: ',name)
    os.environ['bin'] = str(bin)
    os.environ['out'] = str(out)
    
    if repr == 'pdg':
        os.system('sh joern-export $bin'+ " --repr " + "pdg" + ' --out $out') # cpg 改成 pdg
        try:
            pdg_list = os.listdir(out)
            for pdg in pdg_list:
                if pdg.startswith("0-pdg"):
                    file_path = os.path.join(out, pdg)
                    os.system("mv "+file_path+' '+out+'.dot')
                    os.system("rm -rf "+out)
                    break
        except:
            pass
    elif repr == 'ddg':
        os.system('sh joern-export $bin'+ " --repr " + "ddg" + ' --out $out') # cpg 改成 ddg
    else:
        pwd = os.getcwd()
        if out[-4:] != 'json':
            out += '.json'
        joern_process = subprocess.Popen(["./joern"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, shell=True, encoding='utf-8')
        import_cpg_cmd = f"importCpg(\"{bin}\")\r"
        script_path = f"{pwd}/graph-for-funcs.sc"
        run_script_cmd = f"cpg.runScript(\"{script_path}\").toString() |> \"{out}\"\r" #json
        cmd = import_cpg_cmd + run_script_cmd
        ret , err = joern_process.communicate(cmd)
        print(ret,err)

    len_outdir = len(glob.glob(outdir + '*'))
    print('--------------> len of outdir ', len_outdir)
    with open(record_txt, 'a+') as f:
        f.writelines(name+'\n')

def joern_export_new(cwe, bin, outdir, repr):
    outdir = os.path.join(outdir, cwe)
    # record_txt =  os.path.join(outdir,"export_res.txt")
    # if not os.path.exists(record_txt):
    #     os.system("touch "+record_txt)
    # with open(record_txt,'r') as f:
    #     rec_list = f.readlines()

    repo_name = bin.split('/')[-2]
    # if repo_name != 'rizinorg':
    #     return

    name = bin.split('/')[-1].replace(".c","")
    bin = os.path.join(outdir, repo_name, name + '.bin')
    out = os.path.join(outdir, repo_name, name)
    # if name+'\n' in rec_list:
    #     print(" ====> has been processed: ", name)
    #     return
    os.environ['bin'] = str(bin)
    os.environ['out'] = str(out)
    
    # 输出pdg.dot

    if repr == 'pdg':
        if os.path.exists(out+'.dot'):
            return
        print(' ----> now processing: ',cwe, name)
        os.system('sh joern-export $bin'+ " --repr " + "pdg" + ' --out $out') # cpg 改成 pdg
        try:
            pdg_list = os.listdir(out)
            for pdg in pdg_list:
                if pdg.startswith("0-pdg"):
                    file_path = os.path.join(out, pdg)
                    os.system("mv "+file_path+' '+out+'.dot')
                    os.system("rm -rf "+out)
                    break
        except:
            pass
    elif repr == 'ddg':
        os.system('sh joern-export $bin'+ " --repr " + "ddg" + ' --out $out') # cpg 改成 ddg
    else:
        record_txt = '/home/m2024-djj/Vulrag/rag/fail.txt'
        with open(record_txt,'r') as f:
            rec_list = f.readlines()
        if bin + '\n' in rec_list:
            return


        pwd = os.getcwd()
        if out[-4:] != 'json':
            out += '.json'
        if os.path.exists(out):
            return
        # 创建joern进程，允许发送命令、接收输出
        joern_process = subprocess.Popen(["./joern"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, shell=True, encoding='utf-8')
        import_cpg_cmd = f"importCpg(\"{bin}\")\n"
        # 脚本路径
        script_path = f"{pwd}/graph-for-funcs.sc"
        # 执行脚本
        run_script_cmd = f"cpg.runScript(\"{script_path}\").toString() |> \"{out}\"\n" #json
        cmd = import_cpg_cmd + run_script_cmd
        print(cmd)
        ret , err = joern_process.communicate(cmd)
        # 记录没成功的
        if not os.path.exists(out):
            with open(os.path.join(outdir, 'fail_generate.txt'), 'a+') as f:
                f.writelines(out + '\n')


    # len_outdir = len(glob.glob(outdir + '*'))
    # print('--------------> len of outdir ', len_outdir)
    # with open(record_txt, 'a+') as f:
    #     f.writelines(name+'\n')



def main():
    # joern_path = '/home/kyle/joern/joern-1.1.172/joern-cli' 改
    joern_path = '/home/m2024-djj/Vulrag/joern-cli'
    os.chdir(joern_path)
    # 解析得到type(-t，默认export）、repr(-r，默认lineinf_json)
    args = parse_options()
    type = args.type
    repr = args.repr
    # -i，-o
    input_path = args.input
    output_path = args.output

    pool_num = 6
    pool = Pool(pool_num)

    # glob.glob用于匹配所有以.c结尾的文件
    if type == 'parse':
        # files = get_all_file(input_path)
        # files = glob.glob(input_path + '*.c')
        # pool.map(partial(joern_parse, outdir = output_path), files)

        for cwe in tqdm(os.listdir(input_path), desc = 'processing'):
            files = []
            cwe_path = os.path.join(input_path, cwe)
            for repo_name in (os.listdir(cwe_path)):
                repo_path = os.path.join(cwe_path, repo_name)
                for func_name in os.listdir(repo_path):
                    func_path = os.path.join(repo_path, func_name)
                    files.append(func_path)
            for file in files:
                joern_parse_new(cwe, file, output_path)
        # pool.map(partial(joern_parse_new, outdir = output_path), files)

    elif type == 'export':
        for cwe in tqdm(os.listdir(input_path), desc = 'processing'):
            if cwe != 'CWE-125':
                continue
            files = []
            cwe_path = os.path.join(input_path, cwe)
            for repo_name in (os.listdir(cwe_path)):
                repo_path = os.path.join(cwe_path, repo_name)
                for func_name in os.listdir(repo_path):
                    func_path = os.path.join(repo_path, func_name)
                    files.append(func_path)
            # # export + pdg
            for file in tqdm(files, desc=f'processing-{cwe}'):
                joern_export_new(cwe, file, output_path, repr=repr)
            # pool.map(partial(joern_export_new, cwe, outdir=output_path, repr=repr), files)
            with open(os.path.join('/home/m2024-djj/Vulrag/rag/output_bin', 'finish_cwe.txt'), 'a+') as f:
                f.writelines(cwe + '\n')
        # bins = glob.glob(input_path + '*.bin')
        # pool.map(partial(joern_export, outdir=output_path, repr=repr), bins)


    else:
        print('Type error!')    

if __name__ == '__main__':
    main()

