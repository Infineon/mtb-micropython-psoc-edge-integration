import argparse, os

class InfoBuilder:

    def __init__(self, prj_dir, metafiles_dir, suffix, build_dir=None):
        '''
        Initialize the InfoBuilder with project paths and settings.
        Args:
            prj_dir (str):       Path of the target project from an MTB multi project application.
            metafiles_dir (str): Directory containing the output build files (.cflags, .ldflags, etc.). 
                                 Usually the build directory of the target project.
            suffix (str):        Suffix to append to the output files.
            build_dir (str):     Directory to store the output processed build info files. 
                                 If not set, defaults to metafiles_dir. 
        '''
        self.prj_dir = prj_dir
        self.metafiles_dir = metafiles_dir
        self.suffix = suffix
        if build_dir is None:
            self.build_dir = self.metafiles_dir
        else:
            self.build_dir = build_dir

        # Each file type has its own build info function
        self.file_type_builder = {
            ".defines" : self.filter_defines,
            ".cflags"  : self.filter_c_cxx_flags, 
            ".cxxflags": self.filter_c_cxx_flags, 
            ".ldflags" : self.filter_replace_path_ldflags, 
            ".ldlibs"  : self.replace_path_ldlibs,  
            ".ninja"   : self.find_extract_inclist_ninja, 
            ".elf.rsp" : self.replace_path_elf_rsp, 
        }

    def build_info(self, metafile_list):
        '''
        Process a list of metafiles and generate corresponding build info files.
        For each metafile, determine its type and call the appropriate builder function.
        If the metafile type is unsupported, skip it with a message.
        Args:
            metafile_list (list): List of paths to metafiles to process.
        '''
        for metafile in metafile_list:
            builder_func = self.get_builder(metafile)
            metafile_content = InfoBuilder.get_content(metafile)
            build_info_content = builder_func(metafile_content)
            self.save(metafile, build_info_content)

    @staticmethod
    def get_content(file):
        ''' 
        Read the content of a file and return it as a string.
        Args:
            file (str): Path to the file.
        Returns:
            str: Content of the file.
        '''
        with open(file, "r") as f:
            f_content = f.read()

        return f_content

    def get_builder(self, metafile):
        '''
        Determine the type of metafile and return the corresponding builder function.
        First we remove the path to get only the file name.
        Then we check the file extension against the supported types.
        If no match is found, return None.

        Args:
            metafile (str): Path to the metafile.
        Returns:
            function or None: Corresponding builder function or None if unsupported.
        '''
        metafile_name = os.path.basename(metafile)

        extensions_list = list(self.file_type_builder.keys())
        
        for ext in extensions_list:
            if metafile_name.endswith(ext):
                return self.file_type_builder[ext]
        
        raise Exception(f"Unsupported metafile type: {metafile_name}")

    def save(self, metafile, content):
        '''
        Save the processed content to a file in the build directory with the specified suffix.
        
        Args:
            metafile (str): Path to the original metafile.
            content (str): Processed content to save in the output file.
        '''
        metafile_name = os.path.basename(metafile)
        out_file = f"{metafile_name}{self.suffix}"

        out_path = os.path.join(self.build_dir, out_file)

        with open(out_path, "w") as f:
            f.write(" ".join(content) + "\n")

    def filter_defines(self, dot_defines_content):
        ''' 
        The defines do not need any processing.
        This is just to keep the same interface as the other functions.
        Args:
            dot_defines_content (str): Content of the .defines file.
        Returns:
            list: List of defines.
        '''
        return dot_defines_content.split()

    def filter_c_cxx_flags(self, dot_c_ccx_flags_content):
        ''' 
        Retrieve the compiler flags from the .cflags file.
        The flags are contained between the '-c' and the 
        '-MMD' flag.

        Args:
            dot_c_ccx_flags_content (str): Content of the .cflags or .cxxflags file.
        Returns:
            list: List of filtered compiler flags.
        '''
        c_ccx_flags_list = dot_c_ccx_flags_content.split()
        start_index = c_ccx_flags_list.index("-c") + 1 # next is the first element
        end_index = c_ccx_flags_list.index("-MMD")     # '-MMD' is the last element

        return c_ccx_flags_list[start_index:end_index]

    def filter_replace_path_ldflags(self, dot_ldflags_content):
        ''' 
        Retrieve the linker flags from the .ldflags file.
        Replaces the mtb project relative paths to the micropython
        project relative paths.
        Args:
            dot_ldflags_content (str): Content of the .ldflags file.
        Returns:
            list: List of filtered and path-updated linker flags.
        '''
        link_cmd_list = dot_ldflags_content.split()
        link_script_file_param = [item for item in link_cmd_list if item.startswith("-T")] # The linker script is the last element we want to keep
        end_index = link_cmd_list.index(link_script_file_param[0]) + 1 
        filtered_ld_flags = link_cmd_list[:end_index]

        for i, item in enumerate(filtered_ld_flags):
            
            # Additional linker library search paths
            if item == "-L":
                path = filtered_ld_flags[i + 1]  # The path is the next item
                new_path = os.path.join(self.prj_dir, path)
                filtered_ld_flags[i + 1] = new_path  # Update the path in the list

            # Linker script path
            if item.startswith("-T"):
                path = item[3:]  # The path is the part after -T"
                new_path = "-T\"" + os.path.join(self.prj_dir, path)
                filtered_ld_flags[i] = new_path  # Update the path in the list

        return filtered_ld_flags

    def replace_path_ldlibs(self, dot_ldflags_content):
        ''' 
        Retrieve additional linking libraries or objects from the .ldlibs file.
        Replaces the mtb project relative paths to the micropython
        project relative paths.
        '''
        ld_libs_list = dot_ldflags_content.split()

        for i, item in enumerate(ld_libs_list):
            new_path = os.path.join(self.prj_dir, item)
            ld_libs_list[i] = new_path
           
        return ld_libs_list

    def find_extract_inclist_ninja(self, dot_ninja_content):
        '''
        Parse the .ninja file content and extract the include paths from the incflags section.
        The includes are not present in .includes file.
        Each of the include path is in a new line starting with '$' and ending with '$'.
        Those scape characters need to be removed, together with the leading and trailing spaces.

        Args: 
            dot_ninja_content (str): Content of the .ninja file.
        Returns:
            list: List of include paths with -I prefix.
        '''
        dot_nina_file_lines = dot_ninja_content.splitlines()

        inc_list = []
        in_incflags_section = False # Tracking the include flags section

        for line in dot_nina_file_lines:
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
                # Remove '$' prefix and '$' suffix, then strip spaces
                include_path = line[1:].rstrip('$').strip()
                # Append the project path to the include path
                path = include_path[len("-I"):]  # The path is the part after -T"
                new_inc_path = "-I" + os.path.join(self.prj_dir, path)
                inc_list.append(new_inc_path)

        return inc_list
    
    def replace_path_elf_rsp(self, elf_rsp_content):
        '''
        Parse the .elf.rsp file content and replace absolute and relative paths
        to be relative to the micropython project directory.
        Also removes main.o object from the list.

        Args:
            elf_rsp_content (str): Content of the .elf.rsp file.
        Returns:
            list: List of object files with updated paths.
        '''
        rsp_list = elf_rsp_content.split()
        obj_list = []
        for item in rsp_list:

            # Do not add main.o
            # This project main is just used for the ns build.
            if item.endswith('/main.o') or item == 'main.o':
                continue

            # Check if the path is absolute
            if os.path.isabs(item):
                obj_list.append(os.path.relpath(item))  

            else:
                obj_list.append(os.path.join(self.prj_dir, item))

        return obj_list
                 

def parser():
    '''
    Parse command line arguments.

    Returns:
            argparse.Namespace: Parsed arguments.
    '''
    parser = argparse.ArgumentParser(description="Utility to retrieve ModusToolbox build info")
    parser.add_argument("build_metafiles", nargs='*', help="List of build metafiles to process")
    parser.add_argument("--prj-dir", type=str, help="Path of the target project from an MTB multi project application")
    parser.add_argument("--metafiles-dir", type=str,  help="Directory containing the output build files (.cflags, .ldflags, etc.). Usually the build directory of the target project.")
    parser.add_argument("--build_dir", type=str, default=None, help="Directory to store the output processed build info files.")
    parser.add_argument("--suffix", type=str, help="Suffix to append to the output files")

    return parser.parse_args()

if __name__ == "__main__":
    args = parser()
    info_builder = InfoBuilder(args.prj_dir, args.metafiles_dir, args.suffix, args.build_dir)
    info_builder.build_info(args.build_metafiles)