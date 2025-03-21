import sys
import json
import os
import subprocess
import re

from get_metal_arguments import *
from fix_up_metal_shader import *

##
def get_shader_bindings(
        slang_shader_file_path,
        attachment_names,
        shader_resource_names):
    
    file = open(slang_shader_file_path, 'rb')
    file_content = str(file.read())
    file.close()

    bindings = []

    last_render_target_index = -1
    start = 0
    while True:
        output_target_start = file_content.find('SV_TARGET', start)
        if output_target_start < 0:
            break
        
        output_target_name_start = file_content.rfind('{', start, output_target_start)
        output_target_name_end = file_content.find('}', output_target_name_start)
        output_target_struct = file_content[output_target_name_start+1:output_target_name_end]
        lines = output_target_struct.split(';')
        for line in lines:
            tokens = line.split()
            index = 0
            for token in tokens:
                if token.find('SV_TARGET') >= 0:
                    print('token {}'.format(token))
                    index_str = token[len('SV_TARGET'):]
                    binding_index = int(index_str)
                    last_render_target_index = max(binding_index, last_render_target_index)
                    name = tokens[index-2].strip()

                    binding_info = {}
                    binding_info['type'] = 'RWTexture2D<float4>'
                    binding_info['shader-name'] = name
                    binding_info['name'] = attachment_names[binding_index] 
                    binding_info['index'] = binding_index
                    binding_info['set'] = 0
                    bindings.append(binding_info)

                index += 1
        start = output_target_name_end

    start = 0
    while True:
        binding_start = file_content.find('[[vk::binding(', start)
        if binding_start < 0:
            break
        
        # binding string
        binding_end = file_content.find(';', binding_start)
        binding_str = file_content[binding_start:binding_end]
        binding_index_start = binding_str.find('(') + 1
        binding_index_end = binding_str.find(',')
        binding_set_start = binding_index_end + 1
        binding_set_end = binding_str.find(')', binding_set_start)
        
        # binding index and set index
        index_set_str = binding_str[binding_index_start:binding_set_end]
        index_set = index_set_str.split(sep = ', ')
        binding_index = int(index_set[0])
        binding_set = int(index_set[1])

        binding_data_type_start = binding_set_end + 3
        if binding_str.find('\\n') >= 0:
            binding_data_type_start = binding_str.find('\\n') + 2

        # binding name, type, index, and set index
        variable_str = binding_str[binding_data_type_start:]
        #variable_tokens = variable_str.split(sep = ' \n')
        delimiters = r"[ \n]"
        tokens = variable_str.split()
        variable_tokens = []
        for index in range(len(tokens)):
            if len(tokens[index]) > 0:
                variable_tokens.append(tokens[index])

        binding_info = {}
        binding_info['type'] = variable_tokens[0].strip()
        binding_info['shader-name'] = variable_tokens[1].strip()
        binding_info['index'] = binding_index
        binding_info['set'] = binding_set

        if variable_tokens[0] == 'SamplerState':
            binding_info['name'] = 'sampler'
        elif variable_tokens[0] == 'ConstantBuffer<DefaultUniformData>':
            binding_info['name'] = 'Default Uniform Buffer'
        elif variable_tokens[0].find('<ConstantBufferData>') >= 0 and binding_info['shader-name'] == 'constantBuffer':
            binding_info['name'] = 'Push Constant'

        if binding_set == 0 and last_render_target_index >= 0:
            binding_info['index'] = binding_index + last_render_target_index + 1
            binding_index = binding_index + last_render_target_index + 1

        if binding_set == 0 and binding_index < len(attachment_names):
            binding_info['name'] = attachment_names[binding_index]
        elif binding_set == 1 and binding_index < len(shader_resource_names):
            binding_info['name'] = shader_resource_names[binding_index] 

        bindings.append(binding_info)

        start = binding_end + 1
        if start >= len(file_content):
            break

    return bindings


##
def execute_command(args):
    proc = subprocess.Popen(args = args,
        stdout = subprocess.PIPE,
        stderr = subprocess.PIPE)

    cmd_line = ''
    for arg in args:
        cmd_line += arg
        cmd_line += ' '
    print('{}'.format(cmd_line))

    # wait until done
    output = proc.communicate()
    if str(output[1]).find('error:') > 0 or str(output[1]).find('ERROR:') > 0 or str(output[1]).find('error ') > 0:
        print('!!! ERROR !!!')
        print('{}'.format(str(output[1])))
        raise Exception("Compile Error")
        

    if len(output[0]) > 0 or len(output[1]) > 0:
        print('{}'.format(str(output[0])))
        print('{}'.format(str(output[1])))

##
def compile_native_metal_shaders(
        shader_directory,
        output_directory, 
        base_shader_name,
        shader_lib_target_directory):

    metal_file_name = os.path.join(shader_directory, base_shader_name + ".metal")

    # convert to ir
    output_ir_file = os.path.join(output_directory, base_shader_name + '.air')
    args = [
        'xcrun',
        '-sdk',
        'macosx',
        'metal',
        '-o',
        output_ir_file,
        '-c',
        '-frecord-sources',
        '-gline-tables-only',
        '-fmetal-enable-logging',
        metal_file_name
    ]
    execute_command(args)

    # create metal library file with the ir
    metal_lib_file_path = os.path.join(shader_lib_target_directory, base_shader_name + '.metallib')
    args = [
        'xcrun', 
        '-sdk', 
        'macosx', 
        'metal', 
        '-frecord-sources', 
        '-gline-tables-only',
        '-fmetal-enable-logging',
        '-o', 
        metal_lib_file_path
    ]
    args.append(output_ir_file)
    execute_command(args)

    # create symbol file
    args = [
        'xcrun', 
        '-sdk', 
        'macosx', 
        'metal-dsymutil', 
        '-flat', 
        metal_lib_file_path
    ]
    execute_command(args)

##
def compile_pipeline_shaders():
    top_directory = sys.argv[1]
    target_directory = sys.argv[2]
    shader_lib_target_directory = os.path.join(sys.argv[2], 'shader-output')
    render_job_directory = os.path.join(top_directory, 'render-jobs')
    shader_directory = os.path.join(top_directory, 'shaders')

    print('*** arg 0 {} ***'.format(top_directory))
    print('*** arg 1 {} ***'.format(shader_lib_target_directory))

    file_path = os.path.join(render_job_directory, 'trimmed-ray-trace-render-jobs.json')
    file = open(file_path, 'r')
    file_content = file.read()
    file.close()

    ir_files = []
    metal_files = []
    shader_files = {}
    render_jobs = json.loads(file_content)

    # vertex shaders
    render_jobs['Jobs'].append({'Name': 'Full Triangle Vertex Shader', 'Pipeline': 'full-triangle-vertex-shader.json', 'Type': 'Vertex'})
    render_jobs['Jobs'].append({'Name': 'Draw Mesh Vertex Shader', 'Pipeline': 'draw-mesh-vertex-shader.json', 'Type': 'Vertex'})

    for job in render_jobs['Jobs']:
        shader_type = job['Type']
        if shader_type == 'Copy':
            continue
        
        #if shader_type == 'Ray Trace':
        #    continue

        # open render job file, and get the shader name
        pipeline_file_name = job['Pipeline']
        pipeline_file_extension_start = pipeline_file_name.rfind('.')
        pipeline_base_name = pipeline_file_name[:pipeline_file_extension_start]
        pipeline_type = job['Type']

        full_path = os.path.join(render_job_directory, pipeline_file_name)
        render_job_file = open(full_path, 'r')
        render_job_file_content = render_job_file.read()
        render_job_file.close()
        render_job_info = json.loads(render_job_file_content)
        shader_name = render_job_info['Shader']
        file_extension_start = shader_name.rfind('.')
        base_shader_name = shader_name[:file_extension_start]
        slang_shader_name = base_shader_name + '.slang'
        slang_shader_file_path = os.path.join(shader_directory, slang_shader_name)

        attachment_names = []
        shader_resources = []
        for attachment in render_job_info['Attachments']:
            name = attachment['Name']
            type = attachment['Type']
            if 'ParentJobName' in attachment and (type == 'TextureInput' or type == 'BufferInput'):
                name = attachment['ParentJobName'] + '-' + attachment['Name']
            attachment_names.append(name)

        for shader_resource in render_job_info['ShaderResources']:
            name = shader_resource['name']
            shader_resources.append(name)

        bindings = get_shader_bindings(
            slang_shader_file_path,
            attachment_names,
            shader_resources)
        json_str = json.dumps(bindings, indent=4)
        #json_output_file_path = os.path.join(shader_lib_target_directory, base_shader_name + '-bindings.json')
        json_output_file_path = os.path.join(shader_lib_target_directory, pipeline_base_name + '-bindings.json')
        json_output_file = open(json_output_file_path, 'w')
        json_output_file.write(json_str)
        json_output_file.close()

        # determine main function name
        main_function_name = 'PSMain'
        if shader_type == 'Compute':
            main_function_name = 'CSMain'
        elif shader_type == 'Vertex':
            main_function_name = 'VSMain'
        elif shader_type == 'Ray Trace':
            main_function_name = 'rayGen'

        # create output shader directory if needed
        output_directory = os.path.join(top_directory, 'shader-output') 
        os.makedirs(shader_lib_target_directory, exist_ok=True)

        # slang compile command
        output_file_name = os.path.join(output_directory, base_shader_name + '.spv')
        print('compile {}'.format(output_file_name))
        args = [
            '/Users/dingwings/Downloads/slang-2025.5-macos-aarch64/bin/slangc',
            slang_shader_file_path,
            '-profile', 
            'glsl_450', 
            '-target', 
            'spirv',
            '-fvk-use-entrypoint-name', 
            '-DMETAL',
            '-preserve-params',
            '-fspv-reflect',
            '-entry',
            main_function_name,
            '-o',
            output_file_name
        ]
        execute_command(args)
        
        # spirv-cross to metal
        metal_output_file_name = os.path.join(output_directory, base_shader_name + '.metal')
        args = [
            '/Users/dingwings/projects/SPIRV-Cross/build/Release/spirv-cross',
            output_file_name,
            '--output',
            metal_output_file_name,
            '--msl', 
            '--msl-version',
            '20300',
            '--emit-line-directives'
        ]
        execute_command(args)

        # spirv-cross reflection
        metal_reflection_file_name = os.path.join(output_directory, base_shader_name + '-reflection.json')
        args = [
            '/Users/dingwings/projects/SPIRV-Cross/build/Release/spirv-cross',
            output_file_name,
            '--output',
            metal_reflection_file_name,
            '--reflect'
        ]
        execute_command(args)

        if not metal_output_file_name in shader_files:
            metal_files.append(metal_output_file_name)
        shader_files[metal_output_file_name] = 1

        # fix up ray tracing shader, re-order the arguments, replace traceRayEXT() and replace payload with intersection
        if shader_type == 'Ray Trace':
            file_name_end = metal_output_file_name.rfind('.')
            fixed_output_file_path = metal_output_file_name[:file_name_end] + '-fixed.metal'
            
            # shader arguments from the reflection file
            shader_argument_info = fill_output_reflection_info(
                metal_reflection_file_name,
                full_path
            )

            # fix the shader 
            fixed_output = output_fixed_shader(
                metal_output_file_name,
                shader_argument_info[0]
            )

            # set the fixed up shader to the metal shader  
            file = open(fixed_output_file_path, 'w')
            file.write(fixed_output)
            file.close()
            
            metal_output_file_name = fixed_output_file_path
            print('use fixed up ray tracing shader: \"{}\"'.format(metal_output_file_name))

        # convert metal shader to ir
        output_ir_file = os.path.join(output_directory, base_shader_name + '.air')
        args = [
            'xcrun',
            '-sdk',
            'macosx',
            'metal',
            '-o',
            output_ir_file,
            '-c',
            '-frecord-sources',
            '-gline-tables-only',
            '-fmetal-enable-logging',
            metal_output_file_name
        ]
        execute_command(args)
        ir_files.append(output_ir_file)

        # create metal library file with the ir
        metal_lib_file_path = os.path.join(shader_lib_target_directory, base_shader_name + '.metallib')
        args = [
            'xcrun', 
            '-sdk', 
            'macosx', 
            'metal', 
            '-frecord-sources', 
            '-gline-tables-only',
            '-fmetal-enable-logging',
            '-o', 
            metal_lib_file_path
        ]
        args.append(output_ir_file)
        execute_command(args)

        # create symbol file
        args = [
            'xcrun', 
            '-sdk', 
            'macosx', 
            'metal-dsymutil', 
            '-flat', 
            #'-remove-source', 
            metal_lib_file_path
        ]
        execute_command(args)
        

    '''
    compile_native_metal_shaders(
        shader_directory,
        output_directory,
        'indirect-draw-command-compute',
        shader_lib_target_directory)
    '''
    

##
def main():
    print('*** COMPILE SHADERS ***')
    compile_pipeline_shaders()


##
if __name__ == '__main__':
    main()