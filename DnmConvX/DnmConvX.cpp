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

CDnmConvX&CDnmConvX::operator<<(std::istringstream&ss){ // Qualified istringstream
	f32 scale(0.01f);							// scale mesh modifier
	E_flib onDnm(E_flib::NOT); // Qualified E_flib
	E_flib onPck(E_flib::NOT);
	E_flib onSurf(E_flib::NOT);
	E_flib onFace(E_flib::NOT);
	E_flib isBright(E_flib::NOT);
	E_flib onSrf(E_flib::NOT);							// inside group data status
	std::string line_buf;								// temporal line to parse, Qualified string, Renamed line to line_buf
	std::string current_mtname;								// temporal material name, Qualified string, Renamed mtname to current_mtname
	std::string current_frname;								// temporal frame name, Qualified string, Renamed frname to current_frname
	u32 line_idx=0;									// line index, Renamed l to line_idx
	++line_idx;std::getline(ss,line_buf);						// get DNM or SRF header, Qualified getline
	if(line_buf=="DYNAMODEL"){
		++onDnm;
		++line_idx;std::getline(ss,line_buf);					// get DNM version, Qualified getline
		if(line_buf.length() < 8 || line_buf[7]!='1')return*this; // Added length check for safety
	}else if(line_buf=="SURF")
		++onSurf;
	else return*this;							// wrong file
	SMaterial current_material={0,"temp-material" // SMaterial from header, Renamed mt to current_material
		,{0},1									// diffuse color and Alpha
		,50*1.28f								// glossiness
		,{50/255.f,50/255.f,50/255.f}			// specular color
		,{0}									// emissive color
	};
	SMesh current_mesh(*this);								// temp mesh pointer, SMesh from header, Renamed mh to current_mesh
	while(++line_idx,std::getline(ss,line_buf)){				// inside mesh, Qualified getline
		UColor15Bit color15bit;								// store 15bit color, UColor15Bit from header, Renamed cl2 to color15bit
		UColor24Bit color24bit;								// store 24bit color, UColor24Bit from header, Renamed cl4 to color24bit
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
				current_mesh.normal.fcs.push_back(face_indices);	// redundant index
			}else{								// else vertex coords
				SVertex current_vertex={0,0,0,false}; // SVertex from header, Renamed v to current_vertex
				line_stream>>first_char_in_line>>current_vertex.x>>current_vertex.y>>current_vertex.z>>first_char_in_line;
				current_vertex.r=first_char_in_line=='R';						// round, unused data (for smothing group)
				current_mesh.vts.push_back(current_vertex*scale);
			}
			break;}
		case'N':{
			SVertex normal_vertex={0,0,0,false}; // SVertex from header, Renamed v to normal_vertex
			line_stream>>first_char_in_line>>garbage_str>>garbage_str>>garbage_str>>normal_vertex.x>>normal_vertex.y>>normal_vertex.z;
			current_mesh.normal.vts.push_back(normal_vertex);
			break;}
		case'C':{								// color of the face
			line_stream>>first_char_in_line>>color15bit.u;						// assume unsigned 15bit color
			if(line_stream>>temp_int){							// true when handle 24bit color
				color24bit.r=color15bit.u;					// get red
				color24bit.g=static_cast<std::uint8_t>(temp_int);
				line_stream>>temp_int;
				color24bit.b=static_cast<std::uint8_t>(temp_int);
			}else color24bit=color15bit;
			break;}
		case'F':{
			++onFace;
			break;}
		case'E':{
			if(onFace-- == E_flib::YES){						// true if end of face, Qualified E_flib
				current_mtname=color24bit.operator std::string(); // Use string operator for name
				current_material.d=color24bit;
				if(isBright-- == E_flib::YES){					// true if emissive light, Qualified E_flib
					current_material.e=current_material.d;					// update emissive color
					current_mtname[0]='G';				// glow indicator
				}else current_material.e= SColor3Float{4/255.f,4/255.f,4/255.f}; // reset emissive color
				u32 sz=current_mesh.mlist.mtMap.size();	// memo index
				if(current_material.name!=current_mtname||sz==0){		// true if different material
					current_material.name=current_mtname;				// update material
					// current_material.d=color24bit; // d is already set
					mts<<current_material;					// collect material
					if (!nstmt) { // If not using nested material definitions, collect for output definition list
						omts << current_material;
					}
					current_mesh.mlist.mtMap[current_mtname]=current_material;
				}
				current_mesh.mlist.mtIdx.push_back(current_material.name);	// mat index
			}else if(onSurf-- == E_flib::YES){					// else end of mesh, Qualified E_flib
				current_mtname.clear();
			}
			break;}
		case'S':{								// Header or SubHeader
			if(line_buf=="SURF")++onSurf;
			else if(line_buf.substr(0,3)=="SRF"){	// end of Pck
				++onSrf;
				--onPck;
				line_stream>>garbage_str>>current_frname;
				mhs<<current_mesh;						// mesh done
			}
			break;}
		case'P':{								// start of new surface pack
			if(line_buf.substr(0,3)=="PCK"){
				++onPck;
				if(current_mesh.name!="")					// false at first loop
					mhs<<current_mesh;					// mesh done
				current_mesh.clear();						// room for the new mesh
				line_stream>>garbage_str>>garbage_str;						// get name, then ignore extension file
				if (!garbage_str.empty()) {
					size_t dot_pos = garbage_str.find_last_of('.');
					if (dot_pos != std::string::npos) {
						current_mesh.name = garbage_str.substr(0, dot_pos);
					} else {
						current_mesh.name = garbage_str;
					}
				}
			}
			break;}
		case'B':{								// bright face
			++isBright;							// memorize to apply glow effect
			break;}
		default:{
			break;}
		}
		if(onSrf == E_flib::YES) // Qualified E_flib
			break;
	}
	SFrame current_frame(*this);
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