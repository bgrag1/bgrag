import os
import json

# prompt for explanation 
def explanation_prompt(dataset, isvul, file_name):
    if isvul:
        ifvul = '/vul'
    else:
        ifvul = '/novul'
    if dataset == 'secvul':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul'
        func_path = dataset_path + ifvul
    elif dataset == 'cvefixes':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes'
        func_path = dataset_path + ifvul
    elif dataset == 'megavul':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul'
        func_path = dataset_path + ifvul
    func_path = os.path.join(func_path, file_name)

    return f"""# Role
You are a senior cybersecurity analyst. Your task is to perform a root cause analysis based on prior findings.
You have already completed an initial analysis of the "Target Function for Analysis"({file_name[2:].replace('.c', '')}) and concluded that it is vulnerable. The entire history of this analysis, including the retrieved RAG context and your initial verdict, is provided above.

# Background and Task Description
Your new task is to justify your previous conclusion. You must:
1.  **Pinpoint the exact lines of code** within the "Target Function for Analysis"({file_name[2:].replace('.c', '')}) that are responsible for the vulnerability.
2.  **Provide a concise but complete explanation** that explicitly connects your previous findings (from the history) to these specific lines of code.

# Instructions and Output Format
1.  **Review the History**: Carefully re-examine the entire provided history. Your explanation MUST be a logical continuation of the reasoning you started.
2.  **Be Specific**: The code lines you identify must be verbatim from the "Target Function for Analysis"({file_name[2:].replace('.c', '')}) . Do not fabricate or paraphrase code.
""" + """3.  **Strict Output Format**: Provide your response strictly in the following JSON format, with no other text, comments, or explanations before or after it.
{
  "key_code_lines": [<list of the exact code lines in the target function that caused the vulnerability, not line numbers>],
  "explanation": "<a single sentence explaining why>",
}
"""

# prompt for explanation without window
def explanation_prompt_no_window(dataset, isvul, file_name):
    if isvul:
        ifvul = '/vul'
    else:
        ifvul = '/novul'
    if dataset == 'secvul':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul'
        func_path = dataset_path + ifvul
    elif dataset == 'cvefixes':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes'
        func_path = dataset_path + ifvul
    elif dataset == 'megavul':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul'
        func_path = dataset_path + ifvul
    func_path = os.path.join(func_path, file_name)

    with open(func_path, 'r', encoding='utf-8') as f:
        code = f.read()
    return f"""# Role
You are a senior cybersecurity analyst. Your task is to perform a root cause analysis based on prior findings.
You have already completed an initial analysis of the "Target Function for Analysis" and concluded that it is vulnerable.

# Background and Task Description
Your new task is to justify your previous conclusion. You must:
1.  **Pinpoint the exact lines of code and its context** within the "Target Function for Analysis" that are responsible for the vulnerability.
2.  **Provide a concise but complete explanation** that explicitly connects your findings to these specific lines of code.

# Target Function for Analysis
//Code start
{code}
//Code end
""" + """
# Instructions and Output Format
1.  **Be Specific**: The code lines you identify must be verbatim from the "Target Function for Analysis". Do not fabricate or paraphrase code.
2.  **Strict Output Format**: Provide your response strictly in the following JSON format, with no other text, comments, or explanations before or after it.
{
  "key_code_lines": [<list of the code lines in the target function that caused the vulnerability>],
  "explanation": "<a single sentence explaining why>",
}
"""


# prompt for CT
def generate_prompt(dataset, isvul, file_name):
    if isvul:
        ifvul = '/vul'
    else:
        ifvul = '/novul'
    if dataset == 'secvul':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul'
        func_path = dataset_path + ifvul
    elif dataset == 'cvefixes':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes'
        func_path = dataset_path + ifvul
    elif dataset == 'megavul':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul'
        func_path = dataset_path + ifvul
    func_path = os.path.join(func_path, file_name)

    retrive_path = os.path.join(dataset_path, 'retrieve_ret' + ifvul)

    retrived_file = os.path.join(retrive_path, file_name.replace('.c', ''), 'slice_search_31.json')
    if not os.path.exists(retrived_file):
        print(f'未找到{file_name}的检索结果，跳过')
        return False
    with open(func_path, 'r', encoding='utf-8') as f:
        code = f.read()
    with open(retrived_file, 'r', encoding='utf-8') as f:
        retrive_datas = json.load(f)
    retrive_data = retrive_datas[0]  # the first entry is the most relevant
    vul_file_retrieve = retrive_data['func_before']
    vul_file_retrieve_patch = retrive_data['func_after']
    cve_description = retrive_data['description']
    commit_message = retrive_data['commit']
    cwe = retrive_data['cwe']
    with open('/home/m2024-djj/Vulrag/rag/get_cwe_description/cwe_description.json', 'r') as f:
        cwe_description_data = json.load(f)
    if cwe in cwe_description_data.keys():
        cwe_description = cwe_description_data[cwe]['description']

    role = "# Role\nYou are a top C/C++ code security audit expert. You are an expert in identifying vulnerability patterns defined by Common Weakness Enumeration (CWE), skilled in discovering potential security risks through comparative analysis."
    Task = """
# Background and Task Description
I will provide you with detailed information about a known vulnerability case, including the vulnerable code, the patched code, and related metadata. I will also provide a target function for analysis. Your task is to rigorously audit the target function, leveraging the provided case study to determine if it contains a vulnerability identical or highly similar to the one presented in the case.
"""
    

    target_function = f"""
# Target Function for Analysis
//Code Start
{code}
//Code End
"""

    retrieve_inf = f"""
# Known Vulnerability Case
--- VULNERABILITY CASE DETAILS ---
**Vulnerability Category (CWE):** {cwe}:{cwe_description}

**Specific Description (CVE):** {cve_description}

**Commit Message for the Fix:** {commit_message}

**Vulnerable Code Example:**
//Code Start
{vul_file_retrieve}
//Code End

**Patched Code Example:**
//Code Start
{vul_file_retrieve_patch}
//Code End
--- END OF VULNERABILITY CASE DETAILS ---
    """

    output_format = """
# Instructions and Output Format
Based on the provided case, strictly audit the "Target Function for Analysis". Determine if it contains a vulnerability pattern that is identical or highly similar to the one in the "Vulnerable Code Example".
Provide your response strictly in the following JSON format, with no additional text.
{
  "is_vulnerable": true | false,
  "confidence_score": <a float between 0.0 and 1.0 representing your confidence>,
  "brief_reason": "<a single sentence explaining why it is or is not vulnerable>",
}
"""
    return role + Task + target_function +  retrieve_inf + output_format

# prompt for CL
def generate_prompt_new_only_func(dataset, isvul, file_name):
    if isvul:
        ifvul = '/vul'
    else:
        ifvul = '/novul'
    if dataset == 'secvul':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul'
        func_path = dataset_path + ifvul
    elif dataset == 'cvefixes':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes'
        func_path = dataset_path + ifvul
    elif dataset == 'megavul':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul'
        func_path = dataset_path + ifvul
    func_path = os.path.join(func_path, file_name)

    retrive_path = os.path.join(dataset_path, 'retrieve_ret' + ifvul)

    retrived_file = os.path.join(retrive_path, file_name.replace('.c', ''), 'slice_search_31.json')
    if not os.path.exists(retrived_file):
        print(f'未找到{file_name}的检索结果，跳过')
        return False
    with open(func_path, 'r', encoding='utf-8') as f:
        code = f.read()
    with open(retrived_file, 'r', encoding='utf-8') as f:
        retrive_datas = json.load(f)
    retrive_data = retrive_datas[0]  # the first entry is the most relevant
    vul_file_retrieve = retrive_data['func_before']
    vul_file_retrieve_patch = retrive_data['func_after']

    role = "# Role\nYou are a top C/C++ code security audit expert."
    Task = """
# Background and Task Description
Carefully analyze the following C/C++ code snippet. Your task is to determine if it contains any security vulnerabilities. Your asnwer needs to strictly follow the **Output format**. Do not easily judge it as non-vulnerable.
"""
    retrieve_inf = f"""
# Related Vulnerable code snippet
//Code Start
{vul_file_retrieve}
//Code End

# Related patched code snippet
//Code Start
{vul_file_retrieve_patch}
//Code End
    """

    target_function = f"""
# Target Function for Analysis
//Code Start
{code}
//Code End
"""

    output_format = """
#Output Format
Provide your response strictly in the following JSON format, with no additional text.
{
  "is_vulnerable": true | false,
  "confidence_score": <a float between 0.0 and 1.0 representing your confidence>,
  "brief_reason": "<a single sentence explaining why it is or is not vulnerable>"
}
"""
    return role + Task + target_function + retrieve_inf + output_format

# prompt for no rag
def generate_prompt_no_retrieve(dataset, isvul, file_name):
    if isvul:
        ifvul = '/vul'
    else:
        ifvul = '/novul'
    if dataset == 'secvul':
        func_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul' + ifvul
    elif dataset == 'cvefixes':
        func_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes' + ifvul 
    elif dataset == 'megavul':
        func_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul' + ifvul
    func_path = os.path.join(func_path, file_name)

    with open(func_path, 'r', encoding='utf-8') as f:
        code = f.read()


    role = "# Role\nYou are a top C/C++ code security audit expert. You are an expert in identifying vulnerability patterns defined by Common Weakness Enumeration (CWE), skilled in discovering potential security risks through comparative analysis."
    Task = f"""
# Task Description
Carefully analyze the following C/C++ code snippet. Your task is to determine if it contains any security vulnerabilities.

# Target Function for Analysis
//Code Start
{code}
//Code End
"""
    

    output_format = """
# Instructions and Output Format
Provide your response strictly in the following JSON format, with no additional text.
{
  "is_vulnerable": true | false,
  "confidence_score": <a float between 0.0 and 1.0 representing your confidence>,
  "brief_reason": "<a single sentence explaining why it is or is not vulnerable>"
}
"""
    return role + Task + output_format


# prompt for CT-Ext
def generate_prompt_diff(dataset, isvul, file_name):
    if isvul:
        ifvul = '/vul'
    else:
        ifvul = '/novul'
    if dataset == 'secvul':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul'
        func_path = dataset_path + ifvul
    elif dataset == 'cvefixes':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_cvefixes'
        func_path = dataset_path + ifvul
    elif dataset == 'megavul':
        dataset_path = '/home/m2024-djj/Vulrag/rag/test_data_secvul/fliter_megavul'
        func_path = dataset_path + ifvul
    func_path = os.path.join(func_path, file_name)

    retrive_path = os.path.join(dataset_path, 'retrieve_ret' + ifvul)

    retrived_file = os.path.join(retrive_path, file_name.replace('.c', ''), 'slice_search_31.json')
    if not os.path.exists(retrived_file):
        print(f'未找到{file_name}的检索结果，跳过')
        return False
    with open(func_path, 'r', encoding='utf-8') as f:
        code = f.read()
    with open(retrived_file, 'r', encoding='utf-8') as f:
        retrive_datas = json.load(f)
    retrive_data = retrive_datas[0]  # the first entry is the most relevant
    vul_file_retrieve = retrive_data['func_before']
    vul_file_retrieve_patch = retrive_data['func_after']
    cve_description = retrive_data['description']
    commit_message = retrive_data['commit']
    cwe = retrive_data['cwe']
    with open('/home/m2024-djj/Vulrag/rag/get_cwe_description/cwe_description.json', 'r') as f:
        cwe_description_data = json.load(f)
    if cwe in cwe_description_data.keys():
        cwe_description = cwe_description_data[cwe]['description']

    repo, func_name = retrive_data['func_name'].split('/')[0], retrive_data['func_name'].split('/')[1]
    diff_path = '/home/m2024-djj/Vulrag/rag/diff_lines/' + cwe + './' + repo + '/' + func_name + '.json'
    if not os.path.exists(diff_path):
        print(f'未找到{file_name}的diff信息，跳过')
        return False
    # 获取diff信息
    with open(diff_path, 'r') as f:
        diff_lines = json.load(f)
    added_lines, delete_lines = sorted(diff_lines['actual_added_lines']), sorted(diff_lines['actual_deleted_lines'])
    code_line = code.split('\n')
    added_code, deleted_code = [], []
    for i in added_lines:
        if i < len(code_line):
            added_code.append(code_line[i])
    for i in delete_lines:
        if i < len(code_line):
            deleted_code.append(code_line[i])

    role = "# Role\nYou are a top C/C++ code security audit expert. You are an expert in identifying vulnerability patterns defined by Common Weakness Enumeration (CWE), skilled in discovering potential security risks through comparative analysis."
    Task = """
# Background and Task Description
I will provide you with detailed information about a known vulnerability case, including the vulnerable code, the patched code, and related metadata. I will also provide a target function for analysis. Your task is to rigorously audit the target function, leveraging the provided case study to determine if it contains a vulnerability identical or highly similar to the one presented in the case.
Pay special attention to the lines that were deleted and added during the vulnerability fix (diff lines), as these represent the core of the vulnerability and its mitigation.
"""
    retrieve_inf = f"""
# Known Vulnerability Case
--- VULNERABILITY CASE DETAILS ---
**Vulnerability Category (CWE):** {cwe}:{cwe_description}

**Specific Description (CVE):** {cve_description}

**Commit Message for the Fix:** {commit_message}

**Vulnerable Code Example:**
//Code Start
{vul_file_retrieve}
//Code End

**Patched Code Example:**
//Code Start
{vul_file_retrieve_patch}
//Code End

**Key Fix Analysis**
-   **Deleted Lines:** 
    {chr(10).join([f'    {line}' for line in deleted_code])}
-   **Added Lines:** 
    {chr(10).join([f'    {line}' for line in added_code])}
--- END OF VULNERABILITY CASE DETAILS ---
    """

    target_function = f"""
# Target Function for Analysis
//Code Start
{code}
//Code End
"""

    output_format = """
# Instructions and Output Format
Based on the provided case, strictly audit the "Target Function for Analysis". Determine if it contains a vulnerability pattern that is identical or highly similar to the one in the "Vulnerable Code Example".
** Focus on whether the target function contains code patterns or logic that are highly similar to the deleted lines (vulnerable) and lack the added lines (fix) from the "Key Fix Analysis".**
Provide your response strictly in the following JSON format, with no additional text.
{
  "is_vulnerable": true | false,
  "confidence_score": <a float between 0.0 and 1.0 representing your confidence>,
  "brief_reason": "<a single sentence explaining why it is or is not vulnerable>"
}
"""
    return role + Task + retrieve_inf + target_function + output_format


