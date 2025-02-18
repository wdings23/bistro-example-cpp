import sys
import json
import os
import subprocess

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
    if str(output[1]).find('error:') > 0 or str(output[1]).find('ERROR:') > 0:
        print('!!! ERROR !!!')
        print('{}'.format(str(output[1])))
        assert(0)
    if len(output[0]) > 0 or len(output[1]) > 0:
        print('{}'.format(str(output[0])))
        print('{}'.format(str(output[1])))

##
def compile_pipeline_shaders():
    top_directory = sys.argv[1]
    target_directory = sys.argv[2]
    render_job_directory = os.path.join(top_directory, 'render-jobs')
    shader_directory = os.path.join(top_directory, 'shaders')


    file_path = os.path.join(render_job_directory, 'non-ray-trace-render-jobs.json')
    file = open(file_path, 'r')
    file_content = file.read()
    file.close()

    ir_files = []
    metal_files = []
    shader_files = {}
    render_jobs = json.loads(file_content)
    for job in render_jobs['Jobs']:
        shader_type = job['Type']
        if shader_type == 'Copy':
            continue
        
        # open render job file, and get the shader name
        pipeline_file_name = job['Pipeline']
        pipeline_file_extension_start = pipeline_file_name.rfind('.')
        pipeline_base_name = pipeline_file_name[:pipeline_file_extension_start]
        
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

        # determine main function name
        main_function_name = 'PSMain'
        if shader_type == 'Compute':
            main_function_name = 'CSMain'

        # create output shader directory if needed
        output_directory = os.path.join(top_directory, 'shader-output') 
        os.makedirs(output_directory, exist_ok=True)

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
            '-entry',
            main_function_name,
            '-o',
            output_file_name,
            '-g'
        ]
        execute_command(args)
        
        # spirv-cross
        metal_output_file_name = os.path.join(output_directory, base_shader_name + '.metal')
        args = [
            '/Users/dingwings/projects/SPIRV-Cross/build/Release/spirv-cross',
            output_file_name,
            '--output',
            metal_output_file_name,
            '--msl', 
            '--msl-version',
            '20300'
        ]
        execute_command(args)

        if not metal_output_file_name in shader_files:
            metal_files.append(metal_output_file_name)
        shader_files[metal_output_file_name] = 1

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
            metal_output_file_name
        ]
        execute_command(args)
        ir_files.append(output_ir_file)

        # create metal library file with the ir
        metal_lib_file_path = os.path.join(output_directory, pipeline_base_name + '.metallib')
        args = [
            'xcrun', 
            '-sdk', 
            'macosx', 
            'metal', 
            '-frecord-sources', 
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
            '-remove-source', 
            metal_lib_file_path
        ]
        execute_command(args)


    

##
def main():
    print('*** COMPILE SHADERS ***')
    compile_pipeline_shaders()


##
if __name__ == '__main__':
    main()