import json
import sys


##
def parse_arguments(file_content):
    main_start = file_content.find('rayGen')
    argument_start = file_content.find('(', main_start) + 1
    argument_end = file_content.find('{', argument_start) - 1
    argument_str = file_content[argument_start:argument_end]


    argument_array = []
    start = 0
    while True:
        token_end = argument_str.find(']]', start)
        token_end = argument_str.find(',', token_end)
        argument = argument_str[start:token_end]
        argument = argument.lstrip()
        argument_array.append(argument)


        # check for end of arguments
        if token_end >= 0:
            start = token_end + 1
        else:
            break
       


    variable_info_array = []


    num_buffers = 0
    num_textures = 0
    for argument in argument_array:
        tokens = argument.split()


        if tokens[0].find('<') >= 0:
            for i in range(1, len(tokens)):
                if tokens[i].find('>') >= 0:
                    for j in range(1, i + 1):
                        tokens[0] += tokens[j]
                    for j in range(1, i + 1):
                        tokens.remove(tokens[j])
                    break


        variable_info = {}


        # get the type and index token
        variable_type_and_index = 0
        for token_index in range(len(tokens)):
            if tokens[token_index].find('[[') == 0:
                variable_type_and_index = token_index
                break
       
        # append decorations from start to current index
        data_type = ''
        for i in range(variable_type_and_index - 1):
            data_type += tokens[i]
            data_type += ' '
        data_type = data_type.rstrip()


        name = tokens[variable_type_and_index - 1]
        type_and_index = tokens[variable_type_and_index]
        type_and_index_end = type_and_index.find(')')
        if type_and_index_end >= 0:
            type_and_index_str = type_and_index[2:type_and_index_end]
            type_and_index_tokens = type_and_index_str.split('(')


            variable_info['shader_argument_type'] = type_and_index_tokens[0]
            variable_info['shader_argument_index'] = int(type_and_index_tokens[1])


        else:
            if data_type == 'unsupported-built-in-type':
                data_type = 'uint2'
                variable_info['shader_argument_type'] = 'thread_position_in_grid'
                variable_info['shader_argument_index'] = -1


        variable_info['name'] = name
        variable_info['data_type'] = data_type


        variable_info_array.append(variable_info)


        if variable_info['shader_argument_type'] == 'texture':
            num_textures += 1
        elif variable_info['shader_argument_type'] == 'buffer':
            num_buffers += 1
        else:
            pass


    return variable_info_array, argument_start, argument_end


##
def fill_output_reflection_info(spv_reflection_file_path, pipeline_file_path):
    ret = []
   
    file = open(spv_reflection_file_path, 'r')
    file_content = file.read()
    file.close()
    spv_dict = json.loads(file_content)


    file = open(pipeline_file_path, 'r')
    file_content = file.read()
    file.close()
    pipeline_dict = json.loads(file_content)


    categories = ['separate_images', 'images', 'ssbos', 'ubos', 'acceleration_structures']


    # go through the categories from the spirv-cross reflections and get the matching names
    # from the pipeline file
    for category in categories:
        if not category in spv_dict:
            continue


        data_infos = spv_dict[category]
        for data_info in data_infos:
            set_index = data_info['set']
            binding_index = data_info['binding']
            name = ''
            if set_index == 0:
                name = pipeline_dict['Attachments'][binding_index]['Name']
            elif set_index == 1:
                if binding_index >= len(pipeline_dict['ShaderResources']):
                    # last buffer is always the default uniform buffer
                    name = 'Default Uniform Buffer'
                else:
                    name = pipeline_dict['ShaderResources'][binding_index]['name']
            else:
                assert(0)


            info = {}
            info['name'] = name
            info['set_index'] = set_index
            info['binding_index'] = binding_index
            info['shader_name'] = data_info['name']


            ret.append(info)


    ret.sort(key=lambda item: item['binding_index'] + item['set_index'] * 1000)


    pipeline_type = pipeline_dict['Type']


    return ret, pipeline_type
 


##
def output_trace_ray(
    file_path,
    shader_argument_info):
   
    file = open(file_path, 'r')
    file_content = file.read()
    file.close()

    parsed_output = ''

    buffer_index = 0
    texture_index = 0

    # lay out all the arguments for main0() in order specified by the pipeline file
    argument_str_array = []
    used_metal_argument_info = []
    metal_argument_info, start, end = parse_arguments(file_content)
    for i in range(len(shader_argument_info)):
        shader_argument = shader_argument_info[i]
        argument_str = ''
        for j in range(len(metal_argument_info)):
            if shader_argument['shader_name'] == metal_argument_info[j]['name']:
                argument_str += metal_argument_info[j]['data_type']
                argument_str += ' '
                argument_str += metal_argument_info[j]['name']
                argument_str += '[['
                argument_str += metal_argument_info[j]['shader_argument_type']
                argument_str += '('
                if metal_argument_info[j]['shader_argument_type'] == 'buffer':
                    argument_str += '{}'.format(buffer_index)
                    buffer_index += 1
                else:
                    argument_str += '{}'.format(texture_index)
                    texture_index += 1
                argument_str += ')]]'
                argument_str_array.append(argument_str)


                used_metal_argument_info.append(j)


                break
       
        # default uniform buffer is appeneded later on so it doesn't show up in the pipeline file
        if argument_str == '':
            if shader_argument['name'] == 'Default Uniform Buffer':
                for j in range(len(metal_argument_info)):
                    if metal_argument_info[j]['name'].find('defaultUniform') >= 0:
                        argument_str = '{} {} [[buffer({})]]'.format(
                            metal_argument_info[j]['data_type'],
                            metal_argument_info[j]['name'],
                            buffer_index)
                        buffer_index += 1
                        argument_str_array.append(argument_str)
                        used_metal_argument_info.append(j)
                        break

                if len(argument_str) <= 0:
                    assert(0)
            else:
                assert(0)

    # any left-over metal main0() arguments
    if len(used_metal_argument_info) < len(metal_argument_info):
        for i in range(len(metal_argument_info)):
            if not i in used_metal_argument_info:
                argument_str = '{} {} [[{}]]'.format(
                    metal_argument_info[i]['data_type'],
                    metal_argument_info[i]['name'],
                    metal_argument_info[i]['shader_argument_type']
                )
                argument_str_array.append(argument_str)

    # start of content
    parsed_output += file_content[0:start]
    parsed_output += '\n'

    # output ordered argument
    for index in range(len(argument_str_array)):
        parsed_output += '    '
        parsed_output += argument_str_array[index]

        print('argument {} \"{}\"'.format(index, argument_str_array[index]))

        if index < len(argument_str_array) - 1:
            parsed_output += ',\n'
        else:
            parsed_output += '\n)\n'

    start = end

    while True:
        ray_payload_start = file_content.find('p = RayPayload{', start)
        if ray_payload_start >= 0:
            ray_payload_end = file_content.find(';', ray_payload_start)
            parsed_output += file_content[start:ray_payload_start]
            start = ray_payload_end + 1

        trace_ray_start = file_content.find('traceRayEXT', start)
        trace_ray_end = file_content.find(';', trace_ray_start + 1) + 1

        if trace_ray_start >= 0:
            parsed_output += file_content[start:trace_ray_start]

            start_paren = file_content.find('(', trace_ray_start)
            end_paren = file_content.find(';', start_paren)
            parameters = file_content[start_paren + 1:end_paren]
            all_parameters = parameters.split(',')
           
            # parameters:
            # 6: origin
            # 7: t_min
            # 8: direction
            # 9: t_max

            parameter_str = []
            for i in range(6, 10):
                parameter = all_parameters[i]
                tokens = parameter.split()
                parameter_str.append(tokens[0])
           
            space_index = 1
            while True:
                c = file_content[trace_ray_start-space_index]
                if c != '\t' and c != ' ':
                    space_index -= 1
                    break
                space_index += 1


            output_str = 'raytracing::ray r;\n'
            for i in range(space_index):
                output_str += ' '
            output_str += 'r.origin = {} + {} * 0.1f;\n'.format(parameter_str[0], parameter_str[2])
            for i in range(space_index):
                output_str += ' '
            output_str += 'r.min_distance = {};\n'.format(parameter_str[1])
            for i in range(space_index):
                output_str += ' '
            output_str += 'r.direction = {};\n'.format(parameter_str[2])
            for i in range(space_index):
                output_str += ' '
            #output_str += 'r.max_distance = {};\n'.format(parameter_str[3])
            output_str += 'r.max_distance = FLT_MAX;\n'
            for i in range(space_index):
                output_str += ' '
            output_str += 'raytracing::intersector<raytracing::triangle_data, raytracing::instancing, raytracing::world_space_data, raytracing::extended_limits> inter;\n'
            for i in range(space_index):
                output_str += ' '
            output_str += 'inter.assume_geometry_type(raytracing::geometry_type::triangle);\n'
            for i in range(space_index):
                output_str += ' '
            output_str += 'inter.force_opacity(raytracing::forced_opacity::opaque);\n'
            for i in range(space_index):
                output_str += ' '
            output_str += 'inter.accept_any_intersection(false);\n'
            for i in range(space_index):
                output_str += ' '
            output_str += 'auto intersection = inter.intersect(r, scene, 0xff);\n'

            ray_trace_function_end = file_content.find(';', end_paren)

            parsed_output += output_str
            start = trace_ray_end

        intersect_condition_start = file_content.find('if (p.mOutput.w > 0.0)', trace_ray_start)
        if intersect_condition_start >= 0:
            parsed_output += file_content[ray_trace_function_end + 1:intersect_condition_start]
            start_statement = file_content.find('{', intersect_condition_start)

            parsed_output += 'if (intersection.distance > 0.0) '
            start = start_statement
        else:
            parsed_output += file_content[start:]
            break
   
    return parsed_output

##
def output_fixed_shader(
    file_path,
    shader_argument_info):

    parsed_output = output_trace_ray(file_path, shader_argument_info)

    parsed_output = parsed_output.replace('p.mOutput.x', 'intersection.distance')
    parsed_output = parsed_output.replace('p.mOutput.y', 'intersection.primitive_id')
    parsed_output = parsed_output.replace('p.mOutput.z', 'intersection.instance_id')
    parsed_output = parsed_output.replace('p.mBarycentricCoord', 'float3(intersection.triangle_barycentric_coord.x, intersection.triangle_barycentric_coord.y, 1.0 - intersection.triangle_barycentric_coord.x - intersection.triangle_barycentric_coord.y)')
    parsed_output = parsed_output.replace('unknown', 'kernel')
    parsed_output = parsed_output.replace('unsupported-built-in-type', 'uint2')
    parsed_output = parsed_output.replace('unsupported-built-in', 'thread_position_in_grid')

    return parsed_output



##
def main():
    file_path = sys.argv[1]
    start0 = file_path.find('\\')
    start1 = file_path.find('/')
    start = start0
    if start0 < 0 or start1 < start0:
        start = start1
    if start >= 0:
        start += 1
    else:
        start = 0
    file_extension_start = file_path.rfind('.')
    file_base_name = file_path[start:file_extension_start]


    print('basename: {}'.format(file_base_name))


    shader_argument_info, pipeline_type = fill_output_reflection_info(
        file_base_name + '-reflection.json',
        file_base_name + '.json'
    )


    parsed_output = output_trace_ray(
        file_base_name + '.metal',
        shader_argument_info,
        'rayGen')
   
    parsed_output = parsed_output.replace('p.mOutput.x', 'intersection.distance')
    parsed_output = parsed_output.replace('p.mOutput.y', 'intersection.primitive_id')
    parsed_output = parsed_output.replace('p.mOutput.z', 'intersection.instance_id')
    parsed_output = parsed_output.replace('p.mBarycentricCoord', 'float3(intersection.triangle_barycentric_coord.x, intersection.triangle_barycentric_coord.y, 1.0 - intersection.triangle_barycentric_coord.x - intersection.triangle_barycentric_coord.y)')
    parsed_output = parsed_output.replace('unknown', 'kernel')
    parsed_output = parsed_output.replace('unsupported-built-in-type', 'uint2')
    parsed_output = parsed_output.replace('unsupported-built-in', 'thread_position_in_grid')


    file = open(file_base_name + '-fixed.metal', 'w')
    file.write(parsed_output)
    file.close()


##
if __name__ == '__main__':
    main()



