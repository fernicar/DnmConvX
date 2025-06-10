// Minimal viable change test - Attempt 2
#include "DnmConvX.h"
#include <filesystem> // For std::filesystem::path
#include <string>     // For std::string, std::getline, substr, etc.
#include <vector>     // For std::vector
#include <sstream>    // For std::istringstream, std::stringstream
#include <fstream>    // For std::ifstream, std::ofstream
#include <iostream>   // For std::cout, std::cerr, std::endl
#include <iomanip>    // For std::setw, std::right
#include <map>        // For std::map
#include <algorithm>  // For std::remove_if (if needed for string trimming)

// CDnmConvX constructor: Initializer list is compatible with header changes.
CDnmConvX::CDnmConvX(void):omhs(*this),mhs(*this),frs(*this),aks(*this)
{
	nstmt=nstmh=mnm=false;
}

// CDnmConvX::~CDnmConvX(void){
// }

/**
 * @brief Overloaded stream extraction operator to parse DNM content from an input string stream.
 * This is the core parsing logic for the DNM file format, handling various sections like
 * mesh data (vertices, faces, normals, colors), material definitions, and frame/animation hierarchies.
 * @param ss Input string stream containing the DNM file data.
 * @return Reference to this CDnmConvX object, allowing for chained operations.
 */
CDnmConvX&CDnmConvX::operator<<(std::istringstream&ss){
	// ===== Section: Initialization and Header Parsing =====
	// This section initializes parsing state variables and checks the DNM/SRF file header.
	float scale(0.01f);							//!< Scale factor to apply to mesh vertices.
	E_flib onDnm(E_flib::NOT);					//!< Flag indicating if currently parsing a DNM section.
	E_flib onPck(E_flib::NOT);					//!< Flag indicating if currently parsing a PCK (surface pack) section.
	E_flib onSurf(E_flib::NOT);					//!< Flag indicating if currently parsing a SURF (mesh) section.
	E_flib onFace(E_flib::NOT);					//!< Flag indicating if currently parsing face definitions.
	E_flib isBright(E_flib::NOT);				//!< Flag indicating if the current material should be bright/emissive.
	E_flib onSrf(E_flib::NOT);					//!< Flag indicating if currently parsing an SRF (frame hierarchy) section.
	std::string line_buf;						//!< Buffer to hold the currently read line from the input stream.
	std::string current_mtname;					//!< Stores the name of the current material being processed.
	std::string current_frname;					//!< Stores the name of the current frame being processed.
	std::uint32_t line_idx=0;					//!< Index of the current line being parsed, for debugging or error reporting.

	// Read the first line, expecting DNM or SRF header.
	++line_idx; std::getline(ss,line_buf);
	if(line_buf=="DYNAMODEL"){
		++onDnm; // Set DNM parsing mode.
		++line_idx; std::getline(ss,line_buf);	// Read the next line for DNM version.
		// Check if DNM version is supported (expects "VERSION1").
		if(line_buf.length() < 8 || line_buf.substr(0,8)!="VERSION1"){ // More robust check for "VERSION1"
			std::cerr << "Warning: Unsupported DNM version or malformed header: " << line_buf << std::endl;
			return*this; // Early exit if unsupported version.
		}
	}else if(line_buf=="SURF"){
		++onSurf; // Set SURF parsing mode (typically for single, non-animated meshes).
	}else{
		std::cerr << "Error: File is not a recognized DNM or SURF file. Header: " << line_buf << std::endl;
		return*this; // Early exit if not a recognized file type.
	}
	// Initialize a default material.
	SMaterial current_material={0,"temp-material"
		,{0.8f, 0.8f, 0.8f},1.0f					// Default diffuse gray and Alpha
		,64.0f								    // Default glossiness (specular power)
		,{0.2f,0.2f,0.2f}			            // Default specular color
		,{0.0f,0.0f,0.0f}						// Default emissive color
	};
	SMesh current_mesh(*this);								// Accumulates data for the current mesh being parsed.

	// ===== Section: Main Mesh Data Parsing Loop =====
	// This loop processes lines related to mesh geometry and materials.
	while(++line_idx,std::getline(ss,line_buf)){
		UColor15Bit color15bit;								// Temporary storage for 15-bit color data.
		UColor24Bit color24bit;								// Temporary storage for 24/32-bit color data.
		char first_char_in_line = 0;									// garbage char slot, Renamed t to first_char_in_line, initialized
		std::string garbage_str;								// garbage float slot, Qualified string, Renamed g to garbage_str
		u16 temp_int = 0;								// temp int, Renamed i to temp_int, initialized
		std::istringstream line_stream(line_buf); // Qualified istringstream, Renamed is to line_stream
		switch(line_buf[0]){
		case'V':{								// Vertex coord or vertex color
			if(onFace == E_flib::YES){							// true if face vertex id, Qualified E_flib
				line_stream>>first_char_in_line;
				SFaceIdx face_indices; // SFaceIdx from header, Renamed fc to face_indices
				while(line_stream>>temp_int)face_indices.vfi.push_back(temp_int);
				current_mesh.fcs.push_back(face_indices);
				current_mesh.normal.fcs.push_back(face_indices);	// Assume normal indices initially match face indices.
			}else{								// else vertex coords
				SVertex current_vertex; // Default constructor initializes to (0,0,0,false).
				line_stream>>first_char_in_line>>current_vertex.x>>current_vertex.y>>current_vertex.z;
                // Check for optional 'R' flag (rounded vertex, hint for smooth shading).
                if (line_stream.peek() != std::char_traits<char>::eof() && !std::isspace(static_cast<unsigned char>(line_stream.peek()))) {
                    line_stream >> first_char_in_line; // Consume if more data on the line
                    current_vertex.r = (first_char_in_line == 'R');
                }
				current_mesh.vts.push_back(current_vertex*scale);
			}
			break;}
		case'N':{
			SVertex normal_vertex; // Default constructor.
			// The DNM "N" line format can be: N GARBAGE GARBAGE GARBAGE Nx Ny Nz
			// Or sometimes just: N Nx Ny Nz (though less common in older files)
			// This parsing assumes the longer format with garbage strings.
			line_stream>>first_char_in_line; // Consume 'N'
            // Try to read up to 3 garbage strings. If fewer, it's fine.
            for(int i=0; i<3; ++i) { if(!(line_stream >> garbage_str)) break; }
            line_stream >> normal_vertex.x >> normal_vertex.y >> normal_vertex.z;
			current_mesh.normal.vts.push_back(normal_vertex);
			break;}
		// --- Case C: Color data for the current face ---
		case'C':{
			line_stream>>first_char_in_line>>color15bit.u; // Always read the first value as 15-bit color.
			// Check if there's more data on the line, indicating a 24-bit (3-component) color.
			if(line_stream>>temp_int){
				color24bit.r = color15bit.u; // The first value read into color15bit.u was actually the R component.
				color24bit.g = static_cast<std::uint8_t>(temp_int); // The first temp_int is G.
				if(line_stream>>temp_int) { // Check for B component.
				    color24bit.b = static_cast<std::uint8_t>(temp_int);
                } else {
                    color24bit.b = 0; // Default B if missing (should ideally not happen for valid 24-bit).
                }
                color24bit.a = 255; // Default alpha to opaque for 24-bit colors.
			}else{ // It was only a 15-bit color.
                color24bit=color15bit; // Convert 15-bit to 24-bit RGBA structure.
            }
			break;}
		// --- Case F: Start of a new Face definition block ---
		case'F':{
			++onFace; // Set flag indicating subsequent 'V' lines are face indices.
			break;}
		// --- Case E: End of Face definition block (EOVF) or End of SURF block (EOVF) ---
		case'E':{
			if(onFace-- == E_flib::YES){ // End of current face definition block.
				// --- Material Handling for the completed face ---
				current_mtname=color24bit.operator std::string(); // Generate material name from its color string.
				current_material.d=color24bit; // Set diffuse color from the parsed color.
				if(isBright-- == E_flib::YES){ // If 'B' flag was set for this face.
					current_material.e=current_material.d; // Emissive color is same as diffuse.
					if(!current_mtname.empty()) current_mtname[0]='G'; // Prefix name with 'G' to indicate glow/emissive.
				}else {
                    current_material.e = SColor3Float{4/255.f,4/255.f,4/255.f}; // Default dim emissive color.
                }

                bool new_material_entry = true;
                // Check if a material with this generated name already exists for the current mesh.
				if(current_mesh.mlist.mtMap.count(current_mtname)){
                    new_material_entry = false;
                }
                current_material.name = current_mtname; // Set the final name for current_material.

				if(new_material_entry){ // If it's a new material (by name) for this mesh.
					mts<<current_material; // Add to the global collection of parsed materials.
					if (!nstmt) { // If not using nested material definitions for output...
						omts << current_material; // ...add to the global output material collection.
					}
					current_mesh.mlist.mtMap[current_mtname]=current_material; // Add to this mesh's specific material map.
				}
				current_mesh.mlist.mtIdx.push_back(current_material.name); // Assign this material (by name) to the current face.
			}else if(onSurf-- == E_flib::YES){ // End of current SURF (mesh geometry) block.
				current_mtname.clear(); // Reset current material name.
			}
			break;}
		// --- Case S: SURF (start of mesh geometry section) or SRF (start of frame hierarchy section) ---
		case'S':{
			if(line_buf=="SURF") {
                ++onSurf; // Entering a mesh geometry section.
            }
			else if(line_buf.substr(0,3)=="SRF"){	// Start of Frame/Scene Hierarchy Data.
				++onSrf; // Set SRF mode.
				if(onPck == E_flib::YES) --onPck; // If we were in a PCK, it ends here.

                // The current_mesh is now complete. Add it to the collection of parsed meshes.
                if(!current_mesh.name.empty() || !current_mesh.vts.empty()) {
				mhs<<current_mesh;
                }
				// The SRF line also contains the name of the root frame of this hierarchy.
				line_stream>>garbage_str>>current_frname; // garbage_str gets "SRF", current_frname gets root frame name (often quoted).
			}
			break;}
		// --- Case P: PCK (mesh package; effectively a new mesh definition) ---
		case'P':{
			if(line_buf.substr(0,3)=="PCK"){ // Start of a new surface pack.
				++onPck; // Entering a PCK block.
				if(!current_mesh.name.empty() || !current_mesh.vts.empty()){ // If there's an existing mesh being built...
					mhs<<current_mesh;					// ...save it before starting a new one.
                }
				current_mesh.clear();						// Reset current_mesh for the new data.
				line_stream>>garbage_str>>garbage_str;		// First garbage is "PCK", second is the filename (e.g., "meshfile.dnm").
				if (!garbage_str.empty()) {
                    // Extract mesh name from filename, excluding extension, using std::filesystem.
                    current_mesh.name = std::filesystem::path(garbage_str).stem().string();
				}
			}
			break;}
		// --- Case B: Bright flag for next face (indicates emissive material) ---
		case'B':{
			++isBright;							// Set flag, will be checked when 'E' (End of Face) is processed.
			break;}
		default:{ // Unknown line type
			// Optionally log or handle unknown lines. For now, they are ignored.
			break;}
		}
		if(onSrf == E_flib::YES) // If SRF directive was encountered, break from mesh processing loop.
			break;
	}
    // After loop, if a mesh was being parsed (especially for simple SURF files without SRF section), store it.
    if ((onDnm == E_flib::YES || onSurf == E_flib::YES) && (onSrf == E_flib::NOT) && (!current_mesh.name.empty() || !current_mesh.vts.empty())) {
        mhs << current_mesh;
    }

    // ===== Section: Frame and Animation Data Parsing Loop =====
    // This loop processes lines related to the scene hierarchy and animations (SRF, FIL, CLA, STA, POS, CNT, REL, CLD, END).
	SFrame current_frame(*this); // Temporary SFrame object to accumulate current frame data.
	SAnimationKey& current_anim_key=current_frame.ak;
	while(++line_idx,std::getline(ss,line_buf)){
		std::map<std::string,SFrame>&frame_map_ref=frs.frMap;
		std::istringstream line_stream(line_buf);
		std::string keyword_str;
		line_stream>>keyword_str;
		if(keyword_str=="SRF"){
			++onSrf;
			line_stream>>current_frname;
            current_frame.name = current_frname.substr(1,current_frname.size()-2);
            current_anim_key.name = current_frame.name;
		}else if(keyword_str=="FIL"){
			line_stream>>keyword_str;
            if (!keyword_str.empty()) {
                size_t dot_pos = keyword_str.find_last_of('.');
                if (dot_pos != std::string::npos) {
                    current_frame.mhId = keyword_str.substr(0, dot_pos);
                } else {
                    current_frame.mhId = keyword_str;
                }
            }
		}else if(keyword_str=="CLA"){
			line_stream>>current_anim_key.cla;
		}else if(keyword_str=="STA"){
			line_stream>>current_frame.pos[0]>>current_frame.pos[1]>>current_frame.pos[2];
			f32 t_threshold=20.f;
			for(u08 i_idx=0;i_idx<3;++i_idx)
				if(current_frame.pos[i_idx]>t_threshold||current_frame.pos[i_idx]<-t_threshold)
					current_frame.pos[i_idx]=t_threshold;
			for(u08 i_idx=0;i_idx<3;++i_idx)current_frame.pos[i_idx]*=scale;
			line_stream>>current_frame.tpb[0]>>current_frame.tpb[1]>>current_frame.tpb[2];
			current_frame.tpb[2]*=-1;
			line_stream>>current_frame.disp;
			current_anim_key.poss.push_back(current_frame.pos);
			current_anim_key.tpbs.push_back(current_frame.tpb);
			current_anim_key.disps.push_back(current_frame.disp);
		}else if(keyword_str=="POS"){
			line_stream>>current_frame.pos[0]>>current_frame.pos[1]>>current_frame.pos[2];
			for(u08 i_idx=0;i_idx<3;++i_idx)current_frame.pos[i_idx]*=scale;
			line_stream>>current_frame.tpb[0]>>current_frame.tpb[1]>>current_frame.tpb[2];
			current_frame.tpb[2]*=-1;
			line_stream>>current_frame.disp;
		}else if(keyword_str=="CNT"){
			line_stream>>current_frame.cnt.x>>current_frame.cnt.y>>current_frame.cnt.z;
			current_frame.cnt=current_frame.cnt*scale;
		}else if(keyword_str=="REL"){
			line_stream>>keyword_str;
			if(keyword_str!="DEP")
				std::cout<<"line:"<<std::right<<std::setw(6)<<line_idx
				<<"different REL in: "<<current_frname<<std::endl;
		}else if(keyword_str=="CLD"){
			line_stream>>keyword_str;
			keyword_str=keyword_str.substr(1,keyword_str.size()-2);
			frame_map_ref[keyword_str].nested=true;
			current_frame.frIds.push_back(keyword_str);
		}else if(keyword_str=="END"&&onSrf == E_flib::YES){
			--onSrf;
            current_frame.update_transform_and_animation_center();

            bool merged_away = false;
            if(mnm && current_frame.frIds.size()==1 && current_frame.mhId=="null"){
                if (frame_map_ref.count(current_frame.frIds[0])) {
                    SFrame& child_frame = frame_map_ref[current_frame.frIds[0]];
                    SVertex current_ftm_c_vtx; current_ftm_c_vtx = current_frame.ftm.c;
                    SVertex child_ftm_c_vtx; child_ftm_c_vtx = child_frame.ftm.c;

                    bool no_pos_change = !current_ftm_c_vtx.anyChange() && !child_ftm_c_vtx.anyChange();

                    for(const auto& anim_pos_af3 : current_frame.ak.poss) {
                        SVertex anim_vtx; anim_vtx = anim_pos_af3;
                        if (anim_vtx.anyChange()) { no_pos_change = false; break; }
                    }

                    if(no_pos_change){
                        std::cout << current_frame.name << " will be merged to " << current_frame.frIds[0] << ":\t" << current_frame.cnt.operator std::string() << std::endl;
                        child_frame.ak.parents.insert(child_frame.ak.parents.end(), current_frame.ak.parents.begin(), current_frame.ak.parents.end());
                        child_frame.ak.parents.push_back(current_frame.name);
                        merged_away = true;
                    }
                }
            }

            if (!merged_away) {
                frs << current_frame;
                SFrame& stored_frame = frs.frMap[current_frame.name];

                aks << stored_frame.ak;

                if (!stored_frame.mhId.empty() && stored_frame.mhId != "null") {
                    if (mhs.mhMap.count(stored_frame.mhId)) {
                        SMesh& mh_ref = mhs.mhMap[stored_frame.mhId];

                        if (mh_ref.pcnt && (*mh_ref.pcnt != stored_frame.cnt)) {
                            SMesh cmh_clone(mh_ref);
                            std::string s_prefix="C~";
                            s_prefix+=cmh_clone.name;
                            if(cmh_clone.name.length() > 1 && cmh_clone.name[1]=='~') cmh_clone.name[0]+=1;
                            else cmh_clone.name=s_prefix;

                            cmh_clone.pcnt = &stored_frame.cnt;
                            mhs << cmh_clone;
                            stored_frame.mhId = cmh_clone.name;
                        } else {
                             mh_ref.pcnt = &stored_frame.cnt;
                        }

                        if (!nstmh) {
                            omhs << mhs.mhMap[stored_frame.mhId];
                        }
                    } else {
                        std::cerr << "Error: Mesh ID " << stored_frame.mhId << " not found for frame " << stored_frame.name << std::endl;
                    }
                }
            }
			current_frame.clear();
		}
	}
	return*this;
}

void CDnmConvX::finalizeData() {
    // First, process normals for all loaded meshes
    for (auto& pair_ : mhs.mhMap) {
        SMesh& mesh = pair_.second;
        if (!mesh.name.empty() && !mesh.vts.empty()) {
            mesh.checkNormal(mesh.name);
        }
    }

    // Apply face inversions by index
    for (const auto& inv_idx_str : invfidx) {
        std::istringstream is(inv_idx_str);
        std::string mesh_name;
        std::uint16_t face_idx_to_invert;
        is >> mesh_name >> face_idx_to_invert;
        auto mesh_it = mhs.mhMap.find(mesh_name);
        if (mesh_it != mhs.mhMap.end()) {
            mesh_it->second.invertFace(face_idx_to_invert);
        } else {
            std::cerr << "Warning: Mesh not found for face inversion by index: " << mesh_name << std::endl;
        }
    }

    // Apply face inversions by material
    for (const auto& inv_mat_str : invfmt) {
        std::istringstream is(inv_mat_str);
        std::string mesh_name;
        std::string material_name_to_invert;
        is >> mesh_name >> material_name_to_invert;
        auto mesh_it = mhs.mhMap.find(mesh_name);
        if (mesh_it != mhs.mhMap.end()) {
            mesh_it->second.invertFace(material_name_to_invert);
        } else {
            std::cerr << "Warning: Mesh not found for face inversion by material: " << mesh_name << std::endl;
        }
    }

    // Apply mesh blacklist
    for (const auto& mesh_name_to_blacklist : mhbl) {
        auto mesh_it = mhs.mhMap.find(mesh_name_to_blacklist);
        if (mesh_it != mhs.mhMap.end()) {
            mesh_it->second.clear();
            omhs.mhMap.erase(mesh_name_to_blacklist);
        } else {
            std::cout << "Info: Blacklist Mesh not found (already removed or never existed): " << mesh_name_to_blacklist << std::endl;
        }
    }

    // Apply frame blacklist
    for (const auto& frame_name_to_blacklist : frbl) {
        auto frame_it = frs.frMap.find(frame_name_to_blacklist);
        if (frame_it != frs.frMap.end()) {
            frame_it->second.clear();
            aks.akMap.erase(frame_name_to_blacklist);
        } else {
            std::cout << "Info: Blacklist Frame not found (already removed or never existed): " << frame_name_to_blacklist << std::endl;
        }
    }

    if (!dsmh.empty()) {
        // Logic for dsmh would go here, likely modifying material properties.
        std::cout << "// FinalizeData: Processing DoubleSideMesh (Actual logic for applying to materials/meshes deferred)" << std::endl;
    }
}

std::string CDnmConvX::operator std::string() const { // Make it const
	std::stringstream ss,ss2,ss3; // Qualified stringstream
	ss3<<frs;									// update all frames on output
	ss2<<omhs;									// if any, update all mesh definition
	ss<<"xof 0302txt 0032\n"
		<<"Header{1;0;1;}\n"
		<<"AnimTicksPerSecond{1;}\n\n";			// output D3DX header
	ss<<omts;									// if any, output material definitions
	ss<<ss2.str();								// output mesh definitions
	ss<<ss3.str();								// output nested frames
	ss<<aks;									// output animationkeys
	return ss.str();
}
u16 CDnmConvX::outputToXFile(cstr outPath){
	std::string outFilePath(outPath); // Qualified string
	if(outPath==""){
		outFilePath=inFilePath.substr(0,inFilePath.find_last_of('.'));
		outFilePath+=".x";
	}
	std::ofstream fout(outFilePath.c_str()); // Qualified ofstream
	if(!fout)return E_XWrite;					// error check
	fout << operator std::string(); // Use direct call to string conversion operator
	fout.close();
	return E_NotError;
}
u16 CDnmConvX::inputDnmFile(cstr inPath){
	inFilePath=inPath;
	std::ifstream file(inPath, std::ios_base::binary); // Qualified ifstream, open in binary mode
	if(!file.is_open())return E_DnmRead;
	std::string buf; // Qualified string
	file.seekg(0,std::ios_base::end);						// go to end of file, Qualified ios_base
	std::streamoff szf=file.tellg();					// find the file size, Qualified streamoff
	if (szf > 0) { // Ensure file is not empty
		buf.resize(static_cast<size_t>(szf)); // Resize to actual file size
		file.seekg(0,std::ios_base::beg);						// go to begin of file, Qualified ios_base
		file.read(&buf[0],szf); // populate the string buf
	}
	std::streamsize szr=file.gcount(); // Qualified streamsize
	std::cout<<"file size\t"<<szf<<std::endl; // Qualified cout, endl
	std::cout<<"read size\t"<<szr<<std::endl; // Qualified cout, endl
	std::cout<<"CLRF size\t"<<szf-szr<<std::endl; // Qualified cout, endl
	file.close();
	std::istringstream ss(buf); // Qualified istringstream
	*this<<ss;
	return E_NotError;
}
u16 CDnmConvX::inputIniFile(cstr inPath){
	std::vector<std::string>aPs[3];						// poses for Fighter/Gerwalk/Batloid, Qualified vector, string
	std::string ext(inPath); // Qualified string
	ext=ext.substr(ext.find_last_of('.'),ext.size());
	if(ext!=".ini")return E_IniPath;
	std::ifstream file(inPath); // Qualified ifstream
	if(!file)return E_IniRead;
	std::string line; // Qualified string
	std::vector<std::string>*bl=NULL;							// shortcut to category, Qualified vector, string
	while(std::getline(file,line)){ // Qualified getline
		if(line.size()==0||line[0]=='#')continue;	// skip comment lines
		if(line[0]=='[')bl=NULL;					// init a new category
		if(line=="[Config]")bl=&configs;
		else if(line=="[BlacklistMesh]")bl=&mhbl;
		else if(line=="[BlacklistFrame]")bl=&frbl;
		else if(line=="[DoubleSideMesh]")bl=&dsmh;
		else if(line=="[InvertFaceByIdx]")bl=&invfidx;
		else if(line=="[InvertFaceByMaterial]")bl=&invfmt;
		else if(line=="[PoseF]")bl=&aPs[0];
		else if(line=="[PoseG]")bl=&aPs[1];
		else if(line=="[PoseB]")bl=&aPs[2];
		else if(bl)bl->push_back(line);				// store config in category
	}
	for(const auto& config_str : configs){ // Replaced each with range-based for
		if(config_str=="UseNestedMaterial")nstmt=true;
		else if(config_str=="UseNestedMesh")nstmh=true;
		else if(config_str=="MergeEmptyFrames")mnm=true;
	}
	std::vector<u16>s[3]; // Qualified vector
	for(u16 i=0;i<3;++i)							// a custom animation config
		for(u16 j=0;j<aPs[i].size();++j){
			std::string&l=aPs[i][j];						// shortcut for next line, Qualified string
			std::istringstream ss(l); // Qualified istringstream
			u16 cla,sta,k;u08 g_char; // Renamed g to g_char
			if(j==0&&l.size()&&l[0]=='k'&&ss>>g_char)	// true when getting keys
				while(ss>>k)
					s[i].push_back(k);				// save current pose keyframes
			else if(s[i].size(),ss>>cla>>sta)		// else getting CLA STATUS
				for(const auto& key_val : s[i])					// each current pose keyframe, Replaced each with range-based for
					otl[key_val][cla]=sta;			// save current CLA STATUS
		}
	return E_NotError;
}