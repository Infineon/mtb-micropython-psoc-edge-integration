import argparse, os

DEFAULT_MTB_LIB_DIR = "../../lib/mtb-psoc-edge-libs"
MTB_PRJ_RELATIVE_ROOT_PATH = ".."
DEFAULT_MPY_FLAGS_BUILD_DIR = "../../lib/mtb-psoc-edge-libs/proj_cm33_ns/build/APP_KIT_PSE84_AI/Debug"
DEFAULT_MTB_LIB_PRJ_DIR = "../../lib/mtb-psoc-edge-libs/proj_cm33_ns"

def get_content(file):
    with open(file, "r") as f:
        f_content = f.read()

    return f_content

def get_c_cxx_flags(dot_c_cxx_flags_file):
    ''' 
    Retrieve the compiler flags from the .cflags file.
    The flags are contained between the '-c' and the 
    '-MMD' flag.
    '''
    def find_flags_start(build_cmd_list):
        return build_cmd_list.index("-c") + 1  # next is the first element

    def find_flags_end(build_cmd_list):
        return build_cmd_list.index("-MMD")  # '-MMD' is the last element

    c_ccx_flags_list = get_content(dot_c_cxx_flags_file).split()
    start_index = find_flags_start(c_ccx_flags_list)
    end_index = find_flags_end(c_ccx_flags_list)

    c_cxx_flags = c_ccx_flags_list[start_index:end_index]

    # Dump this content in a file with is called .mpy_cflags
    if dot_c_cxx_flags_file.endswith(".cflags"):
        out_file = ".mpy_cflags"
    elif dot_c_cxx_flags_file.endswith(".cxxflags"):
        out_file = ".mpy_cxxflags"

    with open(os.path.join(DEFAULT_MPY_FLAGS_BUILD_DIR, out_file), "w") as f:
        f.write(" ".join(c_cxx_flags) + "\n")

 
def get_ldflags(dot_ldflags_file, mtb_lib_path=DEFAULT_MTB_LIB_DIR, mtb_prj_path=MTB_PRJ_RELATIVE_ROOT_PATH):
    ''' 
    Retrieve the linker flags from the .ldflags file.
    Replaces the mtb project relative paths to the micropython
    project relative paths.
    '''
    def find_flags_end(link_cmd_list):
        '''
        The linker flags required are considered until the "-T" 
        linker flag, which specifies the location of the linker script
        The map file, objects and output .elf is set by the micropython 
        domain.
        '''
        end_delimiter = "-T"
        link_script_file_param = [item for item in link_cmd_list if item.startswith(end_delimiter)]
        return link_cmd_list.index(link_script_file_param[0]) + 1
   
    def replace_lib_search_path(link_cmd_list, mtb_lib_path, mtb_prj_path):
        '''
        Replace the paths in the linker flags to be relative to the micropython project
        for the library search paths (-L).

        For example: 

            "-L ../bsps/TARGET_APP_KIT_PSE84_AI/"

        will be replaced by

            "-L ../../lib/mtb-psoc-edge-libs/bsps/TARGET_APP_KIT_PSE84_AI/

        being the MTB proj relative path "../" and the MTB lib path "../../lib/mtb-psoc-edge-libs".
        '''

        flag_delimiter = "-L"

        for i, item in enumerate(link_cmd_list):
            # if item is equal to flag_delimiter:   
            if item == flag_delimiter:
                path = link_cmd_list[i + 1]  # The path is the next item
                if path.startswith(mtb_prj_path):
                    new_path = path.replace(mtb_prj_path, mtb_lib_path)
                    link_cmd_list[i + 1] = new_path  # Update the path in the list


    def replace_linker_script_path(link_cmd_list, mtb_lib_path, mtb_prj_path):

        '''
        Replace the paths in the linker flags to be relative to the micropython project
        for the linker script path.

        For example: 

            "-T../bsps/TARGET_APP_KIT_PSE84_AI/linker_script.ld"

        will be replaced by

            "-T../../lib/mtb-psoc-edge-libs/bsps/TARGET_APP_KIT_PSE84_AI/linker_script.ld"

        being the MTB proj relative path "../" and the MTB lib path "../../lib/mtb-psoc-edge-libs".
        '''
        for i, item in enumerate(link_cmd_list):
            if item.startswith("-T"):
                new_path = item.replace(mtb_prj_path, mtb_lib_path)
                link_cmd_list[i] = new_path  # Update the path in the list


    ld_flags_list = get_content(dot_ldflags_file).split()

    end_index = find_flags_end(ld_flags_list)
    replace_lib_search_path(ld_flags_list, mtb_lib_path, mtb_prj_path)
    replace_linker_script_path(ld_flags_list, mtb_lib_path, mtb_prj_path)

    ld_flags = ld_flags_list[:end_index]

    # Dump this content in a file with is called .mpy_ldflags
    with open(os.path.join(DEFAULT_MPY_FLAGS_BUILD_DIR, ".mpy_ldflags"), "w") as f:
        f.write(" ".join(ld_flags) + "\n")

 
def get_ldlibs(dot_ldflags_file, mtb_lib_path=DEFAULT_MTB_LIB_DIR, mtb_prj_path=MTB_PRJ_RELATIVE_ROOT_PATH):
    ''' 
    Retrieve additional linking libraries or objects from the .ldlibs file.
    Replaces the mtb project relative paths to the micropython
    project relative paths.
    '''
    ld_libs_list = get_content(dot_ldflags_file).split()

    for i, item in enumerate(ld_libs_list):
        if item.startswith(mtb_prj_path):
            new_path = item.replace(mtb_prj_path, mtb_lib_path)
            ld_libs_list[i] = new_path  # Update the path in the list

    # Dump this content in a file with is called .mpy_ldlibs
    with open(os.path.join(DEFAULT_MPY_FLAGS_BUILD_DIR, ".mpy_ldlibs"), "w") as f:
        f.write(" ".join(ld_libs_list) + "\n")

def get_inclist(dot_ninja_file):
    
    # We are going to parse the .ninja file and look for the "incflags" entries:
    # Then we will parse each line removing the $ delimiters and stripping the spaces.
    
    inc_list = []
    
    try:
        with open(dot_ninja_file, 'r') as f:
            lines = f.readlines()
        
        # Find the incflags section
        in_incflags_section = False
        for line in lines:
            line = line.strip()
            
            # Start of incflags section
            if line.startswith('incflags = $'):
                in_incflags_section = True
                continue
            
            # End of incflags section (next variable definition or empty line)
            if in_incflags_section and (line == '' or (line != '' and not line.startswith('$ -I') and '=' in line)):
                break
            
            # Process incflags lines
            if in_incflags_section and line.startswith('$ -I'):
                # Remove '$ -I' prefix and '$' suffix, then strip spaces
                include_path = line[4:].rstrip('$').strip()
                if include_path:  # Only add non-empty paths
                    inc_list.append('-I' + include_path)
    
    except FileNotFoundError:
        print(f"Error: Ninja file not found: {app_ninja_file}")
        return
    except Exception as e:
        print(f"Error parsing ninja file: {e}")
        return

    for i, path in enumerate(inc_list):
        if path.startswith("-I"):
            new_path = path.replace("-I", "-I" + DEFAULT_MTB_LIB_PRJ_DIR + "/")
            inc_list[i] = new_path  # Update the path in the list
    
    # Dump this content in a file called .mpy_inclist
    with open(os.path.join(DEFAULT_MPY_FLAGS_BUILD_DIR, ".mpy_inclist"), "w") as f:
        f.write(" ".join(inc_list) + "\n")

def get_objlist(elf_rsp_file):
    
    # Read the .rsp file and process object file paths
    # Convert absolute paths to relative paths to ports/psoc-edge
    # Remove main.o object
    # Replace ../ paths from proj_cm33_ns with relative paths to ports/psoc-edge
    
    obj_list = []
    
    try:
        with open(elf_rsp_file, 'r') as f:
            content = f.read().strip()
        
        # Split the content by spaces to get individual object files
        obj_files = content.split()
        
        for obj_file in obj_files:
            obj_file = obj_file.strip()
            if not obj_file:
                continue
            
            # Skip main.o object
            if obj_file.endswith('/main.o') or obj_file == 'main.o':
                continue
            
            # Convert absolute paths to relative paths
            if obj_file.startswith('/'):
                # Find the mtb-psoc-edge-libs base path in the absolute path
                if '/lib/mtb-psoc-edge-libs/' in obj_file:
                    # Extract the part after mtb-psoc-edge-libs
                    parts = obj_file.split('/lib/mtb-psoc-edge-libs/')
                    if len(parts) > 1:
                        relative_part = parts[1]
                        # Convert to relative path from ports/psoc-edge
                        obj_file = '../../lib/mtb-psoc-edge-libs/' + relative_part
            
            # Handle ../ paths from proj_cm33_ns (these are already relative)
            # These paths like "../proj_cm33_s/nsc_veneer.o" should become "../../lib/mtb-psoc-edge-libs/proj_cm33_s/nsc_veneer.o"
            elif obj_file.startswith('../'):
                # Remove the ../ prefix and add the full relative path from ports/psoc-edge
                relative_part = obj_file[3:]  # Remove "../"
                obj_file = '../../lib/mtb-psoc-edge-libs/' + relative_part
            
            # Add the processed object file to the list
            if obj_file:
                obj_list.append(obj_file)
    
    except FileNotFoundError:
        print(f"Error: Response file not found: {elf_rsp_file}")
        return
    except Exception as e:
        print(f"Error parsing response file: {e}")
        return
    
    # Dump this content in a file called .mpy_objlist
    with open(os.path.join(DEFAULT_MPY_FLAGS_BUILD_DIR, ".mpy_objlist"), "w") as f:
        f.write(" ".join(obj_list) + "\n")

def get_all_flags(file):
    print("Getting all flags...")
    get_c_cxx_flags(os.path.join(DEFAULT_MPY_FLAGS_BUILD_DIR, ".cflags"))
    get_c_cxx_flags(os.path.join(DEFAULT_MPY_FLAGS_BUILD_DIR, ".cxxflags"))
    get_ldflags(os.path.join(DEFAULT_MPY_FLAGS_BUILD_DIR, ".ldflags"))
    get_ldlibs(os.path.join(DEFAULT_MPY_FLAGS_BUILD_DIR, ".ldlibs"))
    dot_ninja_file = [f for f in os.listdir(DEFAULT_MPY_FLAGS_BUILD_DIR) if f.endswith('.ninja')][0]
    get_inclist(os.path.join(DEFAULT_MPY_FLAGS_BUILD_DIR, dot_ninja_file))
    dot_elf_rsp_file = [f for f in os.listdir(DEFAULT_MPY_FLAGS_BUILD_DIR) if f.endswith('.elf.rsp')][0]
    get_objlist(os.path.join(DEFAULT_MPY_FLAGS_BUILD_DIR, dot_elf_rsp_file))

flags_func = {
    "cflags"  : get_c_cxx_flags,
    "cxxflags": get_c_cxx_flags,
    "ldflags" : get_ldflags,
    "ldlibs"  : get_ldlibs,
    "inclist" : get_inclist,
    "objlist" : get_objlist,
    "all"     : get_all_flags,
}

def build_info(type, file=None):
    print("DOING THE FUNCTION TYPE: ", type)
    flags_func[type](file)

def parser():

    cmd_list = list(flags_func.keys())

    parser = argparse.ArgumentParser(description="Utility to retrieve ModusToolbox build info")
    parser.add_argument("flags", choices=cmd_list, help="Type of flags to retrieve")
    parser.add_argument("-f","--file", default=None, type=str, help="File to retrieve the flags from")

    parser.add_argument("--build_dir", type=str, default=DEFAULT_MPY_FLAGS_BUILD_DIR, help="Build directory")
    parser.add_argument("--mtb-lib-dir", type=str, default=DEFAULT_MTB_LIB_DIR, help="Path to the MTB libraries")

    args = parser.parse_args()
    build_info(args.flags, args.file) #args.build_dir)

if __name__ == "__main__":
    parser()