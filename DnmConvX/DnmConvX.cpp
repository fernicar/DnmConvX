#include "DnmConvX.h"

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
		if(line_buf[7]!='1')return*this;
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
		char first_char_in_line;									// garbage char slot, Renamed t to first_char_in_line
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
				color24bit.g=static_cast<std::uint8_t>(temp_int); // Cast temp_int to u08(uint8_t)
				line_stream>>temp_int;
				color24bit.b=static_cast<std::uint8_t>(temp_int); // Cast temp_int to u08(uint8_t)
			}else color24bit=color15bit;
			break;}
		case'F':{
			++onFace;
			break;}
		case'E':{
			if(onFace-- == E_flib::YES){						// true if end of face, Qualified E_flib
				current_mtname=color24bit;						// create unique rgb name
				current_material.d=color24bit;
				if(isBright-- == E_flib::YES){					// true if emissive light, Qualified E_flib
					current_material.e=current_material.d;					// update emissive color
					current_mtname[0]='G';				// glow indicator
				}else current_material.e=4/255.f;				// reset emissive color
				u32 sz=current_mesh.mlist.mtMap.size();	// memo index
				if(current_material.name!=current_mtname||sz==0){		// true if different material
					current_material.name=current_mtname;				// update material
					current_material.d=color24bit;
					mts<<current_material;					// collect material
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
				current_mesh.name=garbage_str.substr(0,garbage_str.find_last_of('.'));
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
	SFrame current_frame(*this); // SFrame from header, Renamed f to current_frame
	SAnimationKey& current_anim_key=current_frame.ak; // SAnimationKey from header, Renamed a to current_anim_key
	while(++line_idx,std::getline(ss,line_buf)){				// inside DNM animation, Qualified getline
		std::map<std::string,SFrame>&frame_map_ref=frs.frMap;		// shortcut for frame map, Qualified map, string, SFrame, Renamed frMap to frame_map_ref
		std::istringstream line_stream(line_buf); // Qualified istringstream, Renamed is to line_stream
		std::string keyword_str;								// temporal string storage, Qualified string, Renamed g to keyword_str
		line_stream>>keyword_str;									// get next keyword
		if(keyword_str=="SRF"){
			++onSrf;
			line_stream>>current_frname;							// get frame name
		}else if(keyword_str=="FIL"){						// FIL
			line_stream>>keyword_str;
			current_frame.mhId=keyword_str.substr(0,keyword_str.find_last_of('.'));	// set mesh name
		}else if(keyword_str=="CLA"){						// CLA
			line_stream>>current_anim_key.cla;							// type of animation
		}else if(keyword_str=="STA"){						// STA
			line_stream>>current_frame.pos[0]>>current_frame.pos[1]>>current_frame.pos[2];
			f32 t_threshold=20.f;							// distance treshold, Renamed t_val to t_threshold
			for(u08 i_idx=0;i_idx<3;++i_idx) // Renamed i to i_idx
				if(current_frame.pos[i_idx]>t_threshold||current_frame.pos[i_idx]<-t_threshold)
					current_frame.pos[i_idx]=t_threshold;					// limit extreme coords
			for(u08 i_idx=0;i_idx<3;++i_idx)current_frame.pos[i_idx]*=scale; // Renamed i to i_idx
			line_stream>>current_frame.tpb[0]>>current_frame.tpb[1]>>current_frame.tpb[2];	// turn, then pitch, then bank
/*			const float e_factor=0.0054931640625f;		// 360/65536 (deg/16bit), Renamed e to e_factor
			if(current_frame.tpb[0]||current_frame.tpb[1]||current_frame.tpb[2])
				std::cout<<"Turn,Pitch,-Bank,frame:"<<std::fixed<<std::setprecision(0) // Qualified cout, fixed, setprecision
					<<std::setw(4)<<std::right<<current_frame.tpb[0]*e_factor // Qualified setw, right
					<<std::setw(4)<<std::right<<current_frame.tpb[1]*e_factor // Qualified setw, right
					<<std::setw(4)<<std::right<<current_frame.tpb[2]*e_factor<<' '<<current_frname<<std::endl; // Qualified setw, right, endl
*/			current_frame.tpb[2]*=-1;						// inverted bank angle
			line_stream>>current_frame.disp;							// mesh display at status anim
			current_anim_key.poss.push_back(current_frame.pos);
			current_anim_key.tpbs.push_back(current_frame.tpb);
			current_anim_key.disps.push_back(current_frame.disp);
		}else if(keyword_str=="POS"){						// POS
			line_stream>>current_frame.pos[0]>>current_frame.pos[1]>>current_frame.pos[2];
			for(u08 i_idx=0;i_idx<3;++i_idx)current_frame.pos[i_idx]*=scale; // Renamed i to i_idx
			line_stream>>current_frame.tpb[0]>>current_frame.tpb[1]>>current_frame.tpb[2];	// turn, then pitch, then bank
/*			const float e_factor=0.0054931640625f;		// 360/65536 (deg/16bit), Renamed e to e_factor
			if(current_frame.tpb[0]||current_frame.tpb[1]||current_frame.tpb[2])
				std::cout<<"Turn,Pitch,-Bank,frame:"<<std::fixed<<std::setprecision(0) // Qualified cout, fixed, setprecision
					<<std::setw(4)<<std::right<<current_frame.tpb[0]*e_factor // Qualified setw, right
					<<std::setw(4)<<std::right<<current_frame.tpb[1]*e_factor // Qualified setw, right
					<<std::setw(4)<<std::right<<current_frame.tpb[2]*e_factor<<' '<<current_frname<<std::endl; // Qualified setw, right, endl
*/			current_frame.tpb[2]*=-1;						// inverted bank angle
			line_stream>>current_frame.disp;							// mesh display at still pos
		}else if(keyword_str=="CNT"){						// CNT
			line_stream>>current_frame.cnt.x>>current_frame.cnt.y>>current_frame.cnt.z;
			current_frame.cnt=current_frame.cnt*scale;
		}else if(keyword_str=="REL"){						// Unknown keyword
			line_stream>>keyword_str;
			if(keyword_str!="DEP")
				std::cout<<"line:"<<std::right<<std::setw(6)<<line_idx // Qualified cout, right, setw
				<<"different REL in: "<<current_frname<<std::endl; // Qualified endl
		}else if(keyword_str=="CLD"){
			line_stream>>keyword_str;
			keyword_str=keyword_str.substr(1,keyword_str.size()-2);			// remove double quot
			frame_map_ref[keyword_str].nested=true;				// set true on nested child
			current_frame.frIds.push_back(keyword_str);				// track nested frame
		}else if(keyword_str=="END"&&onSrf == E_flib::YES){						// End of hierarchy node, Qualified E_flib
			--onSrf;
			f.name=frname.substr(1,frname.size()-2);	// update frame name
			a.name=f.name;						// update frame name to anim
			frs<<f;								// store frame
			f.clear();
		}
	}

	return*this;
}

CDnmConvX::operator std::string(){ // Changed return type to std::string directly
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
	return ss.str(); // Changed to return std::string
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