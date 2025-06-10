#ifndef DNMCONVX_H_
#define DNMCONVX_H_
#include<array>		// std::array<>
#include<string>	// std::string
#include<sstream>	// std::istringstream std::stringstream
#include<iostream>	// std::cout std::endl std::getchar() std::ostream
#include<iomanip>	// std::setprecision() std::hex std::setw() std::setfill() std::right
#include<fstream>	// std::ifstream std::ofstream
#include<vector>	// std::vector<>
#include<map>		// std::map<> std::make_pair()
#include<cmath>		// std::sqrtf() std::cos() std::sin()
#include<cstdint>
#include<functional>
#include<regex>
#include <format> // Added format include
#include <span>   // Added span include

using cstr = const char*;
using c08 = char;
using u08 = std::uint8_t;

using i32 = std::int32_t;
using i16 = std::int16_t;
using u32 = std::uint32_t;
using u16 = std::uint16_t;

using f32 = float;
using ai3 = std::array<i32,3>;
using af3 = std::array<f32,3>;
using af9 = std::array<f32,9>;
using vs = std::vector<std::string>;
using vi = std::vector<i32>;
using vu = std::vector<u16>;
using vf = std::vector<f32>;
using uu = std::map<u16,u16>;

using it_vs_ = vs::const_iterator;
using it_vs = vs::iterator;
using it_vi_ = vi::const_iterator;
using it_vi = vi::iterator;
using it_vu_ = vu::const_iterator;
using it_vu = vu::iterator;
using it_vf_ = vf::const_iterator;
using it_vf = vf::iterator;
using it_uu_ = uu::const_iterator;
using it_uu = uu::iterator;

// Macro definitions removed
// #pragma warning directives removed

enum E_ERROR { // Added space for readability, no functional change
	E_NotError,
	E_IniPath,
	E_IniRead,
	E_MemoryError,
	E_DnmRead,
	E_XWrite,
	E_Error
};

enum class E_flib {NOT=false, YES=true}; // Changed typedef enum to enum class
inline E_flib&operator++(E_flib&b)		{b=E_flib::YES; return b;}				// prefix ++
inline E_flib operator++(E_flib&b,int)	{E_flib t=b;++b;return t;}	// postfix ++
inline E_flib&operator--(E_flib&b)		{b=E_flib::NOT; return b;}				// prefix --
inline E_flib operator--(E_flib&b,int)	{E_flib t=b;--b;return t;}	// postfix --
//inline E_flib&operator=(E_flib&f,const bool&b){return f=b?E_flib::YES:E_flib::NOT} // Adjusted for enum class

class CDnmConvX{
public:
	CDnmConvX(void);
// 	~CDnmConvX(void);
	CDnmConvX&operator<<(std::istringstream&iss);
	[[nodiscard]] operator std::string() const; // Changed to std::string and made const
	void finalizeData(); // New method for INI-based processing
	u16 outputToXFile(cstr outPath="");
	u16 inputDnmFile(cstr inPath);
	u16 inputIniFile(cstr inPath);

private:
friend struct SMaterialList;
friend struct SMapCollMsh;
friend struct SMapCollFrm;
friend struct SMapCollAnim;

struct UColor15Bit{ // Changed from union to struct
	u16 u;								// init helper (max 15bit valid 0x7FFF=32767)
	struct{
		u16 r:5;						// red
		u16 g:5;						// green
		u16 b:5;						// blue
		u16 p:1;						// padding garbage, total=16bit
	};
}; // Removed c15b
struct UColor24Bit{ // Changed from union to struct
	struct{
		u16 r:8;						// red
		u16 g:8;						// green
		u16 b:8;						// blue
		u16 a:8;						// alpha
	};
	u32 u;								// init helper, size 32bit
	UColor24Bit&operator=(const UColor15Bit&c){ // Was c15b
		r=c.r*8+c.r/4;
		g=c.g*8+c.g/4;
		b=c.b*8+c.b/4;
		return*this;}
	[[nodiscard]] std::string operator cstr() const { // Ensure it's const
        return std::format("_{:02x}{:02x}{:02x}_", r, g, b);
	}
}; // Removed c24b
struct SVertex{ // Changed from typedef struct
	f32 x,y,z;bool r;					// rounded (smooth surface)
//	SVertex(const af3&c):x(c[0]),y(c[1]),z(c[2]),r(false){}
	SVertex&operator=(const SVertex&v){
		x=v.x;y=v.y;z=v.z;r=v.r;
		return*this;
	}
	SVertex&operator=(const af3&c){
		x=c[0];y=c[1];z=c[2];r=false;
		return*this;
	}
	SVertex operator-(const SVertex&v) const { // Added const
		SVertex a={x-v.x,y-v.y,z-v.z,false};
		return a;
	}
	SVertex operator+(const SVertex&v) const { // Added const
		SVertex a={x+v.x,y+v.y,z+v.z,false};
		return a;
	}
	[[nodiscard]] std::string operator cstr() const { // Ensure it's const
        return std::format("{:.6f};{:.6f};{:.6f};", x, y, z);
	}
	f32 dot(const SVertex&v){
		return (x*v.x+y*v.y+z*v.z);
	}
	SVertex cross(const SVertex&v){
		SVertex r_val=	{y*v.z-z*v.y // Renamed r to r_val to avoid conflict with SVertex::r
					,z*v.x-x*v.z
					,x*v.y-y*v.x};
		return r_val;
	}
	f32 length(){return std::sqrtf(x*x+y*y+z*z);} // Qualified sqrtf
	void normalize(const std::string&name="",const u16 idx=0){ // Qualified string
		f32 len=length(),mod=1;
		if(x==0&&y==0&&z==0)
			std::cerr<<name<<" V:"<<std::setw(4)<<std::right<<idx<<" invalid vertex normal\n"; // Qualified cerr, setw, right
		else mod=1/len;
		x*=mod;y*=mod;z*=mod;
		return;
	}
	void invert(){x=-x;y=-y;z=-z;}
	bool anyChange(){return x!=0||y!=0||z!=0;}
	void clear(){x=0;y=0;z=0;r=false;}
	SVertex&operator+=(const SVertex&v){
		x+=v.x;y+=v.y;z+=v.z;
		return*this;
	}
	SVertex operator*(const f32&f) const { // Added const
		SVertex n={x*f,y*f,z*f,false};
		return n;
	}
	bool operator!=(const SVertex&v){return (x!=v.x||y!=v.y||z!=v.z);}
	bool operator!=(const af3&v){return (x!=v[0]||y!=v[1]||z!=v[2]);}
}; // Removed vertex
using itvV = std::vector<SVertex>::iterator; // Changed vertex to SVertex, qualified vector
struct SFaceIdx{ // Changed from typedef struct
	std::vector<u16>vfi; // Qualified vector
	void reverseContent(){
		std::vector<u16>r_val(vfi.rbegin(),vfi.rend()); // Qualified vector, renamed r to r_val
		vfi.swap(r_val);
	}
	[[nodiscard]] std::string operator cstr() const { // Ensure it's const
        if (vfi.empty()) return "";
        std::string s = std::format("{};", vfi.size()); // Removed std::fixed and std::setprecision
        for (std::size_t i = 0; i < vfi.size(); ++i) {
            s += std::format("{}", vfi[i]);
            if (i < vfi.size() - 1) {
                s += ",";
            }
        }
        s += ";";
        return s;
	}
}; // Removed faceIdx
using itvFI = std::vector<SFaceIdx>::iterator; // Changed faceIdx to SFaceIdx, qualified vector
struct SColor3Float{ // Changed from typedef struct
	f32 r,g,b;
	SColor3Float&operator=(const UColor24Bit&c24){ // Was c24b
		r=c24.r/255.f;g=c24.g/255.f;b=c24.b/255.f;
		return*this;
	}
	SColor3Float&operator=(const SColor3Float&c){
		r=c.r;g=c.g;b=c.b;
		return*this;
	}
	SColor3Float&operator=(f32 f){
		r=g=b=f;
		return*this;
	}
	[[nodiscard]] std::string operator cstr() const { // Ensure it's const
        return std::format("{:.6f};{:.6f};{:.6f}", r, g, b);
	}
}; // Removed cl3f
struct SMaterial{ // Changed from typedef struct
	u16 i;							// material list index
	std::string name; // Qualified string
	SColor3Float d;							// diffuse RGB, Was cl3f
	f32 a;							// alpha
	f32 g;							// gloss
	SColor3Float s;							// specular RGB, Was cl3f
	SColor3Float e;							// emissive RGB, Was cl3f
	[[nodiscard]] std::string operator cstr() const { // Ensure it's const
        return std::format("Material {} {{\n"
                           "{};{:.6f};;\n"
                           "{:.6f};\n"
                           "{};;\n"
                           "{};;\n}}",
                           name, d.operator std::string(), a, g, s.operator std::string(), e.operator std::string());
	}
}; // Removed material
using itsMT = std::map<std::string,SMaterial>::iterator; // Qualified map, string. Changed material to SMaterial
struct SMaterialList{ // Changed from typedef struct
	SMaterialList(CDnmConvX& c) : p_(c) {}; // Updated constructor, p to p_
	CDnmConvX& p_; // Changed to reference
	std::vector<std::string>mtIdx;						// material index per vertex, Qualified vector, string
	std::map<std::string,SMaterial>mtMap; // Qualified map, string. Changed material to SMaterial
	// Removed default constructor SMaterialList():p(NULL){}
	[[nodiscard]] std::string operator std::string() const { // Made const
        std::string result = std::format("MeshMaterialList {{\n"
                                     "{};\n"
                                     "{};\n",
                                     mtMap.size(), mtIdx.size());

        // The re-indexing `for(auto& pair_ : mtMap){ pair_.second.i=idx++; }`
        // is a non-const operation. It's removed from this const operator.
        // It should be handled by a separate non-const method if material indices need dynamic updates.
        // For this const operator, we assume SMaterial::i is already correctly set.

        for(const auto& val : mtIdx) {
            auto it = mtMap.find(val);
            if (it != mtMap.end()) {
                result += std::format("{},", it->second.i);
            } else {
                // This case should ideally not happen if data is consistent
                result += "ERROR_IDX,";
            }
        }
        if (!mtIdx.empty() && result.back() == ',') result.pop_back(); // Remove last comma
        result += ";;\n";

        // The logic involving p_.omts.mtMap.insert(pair_); is a side effect
        // on an external object (p_) and cannot be part of a const operator
        // unless p_.omts or its map is mutable (which is not assumed).
        // This logic should be handled elsewhere, possibly in a non-const method
        // that prepares data for output.
        // For now, we format based on the current state of p_.nstmt.
        for (const auto& pair_ : mtMap) {
            if (p_.nstmt) {
                result += pair_.second.operator std::string() + "\n";
            } else {
                // Commenting out the modification to external state:
                // p_.omts.mtMap.insert(pair_);
                result += std::format("{{{}}}\n", pair_.second.name);
            }
        }
        result += "}";
        return result;
	}
}; // Removed mlist
struct SMeshNormals{ // Changed from typedef struct
	std::vector<SVertex>vts;						// faces normal coords to vertex normal coords, Qualified vector, Changed vertex to SVertex
	std::vector<SFaceIdx>fcs;						// redundant faces, Qualified vector, Changed faceIdx to SFaceIdx
	void invertNormal(u16 idx){
		vts[idx].invert();
	}
	void invertFace(u16 idx){
		fcs[idx].reverseContent();
	}
	[[nodiscard]] std::string operator cstr() const { // Ensure it's const
        std::string result = std::format("MeshNormals{{\n{}", vts.size());
        result += ";"; // Terminator for vts.size()
        for (const auto& vertex_val : vts) {
            // .operator std::string() is optional if conversion is not ambiguous
            result += std::format("\n{},", vertex_val.operator std::string());
        }
        if (!vts.empty() && result.back() == ',') result.pop_back(); // Remove last comma
        result += ";\n";

        result += std::format("{};", fcs.size());
        for (const auto& face_idx_val : fcs) {
            // .operator std::string() is optional
            result += std::format("\n{},", face_idx_val.operator std::string());
        }
        if (!fcs.empty() && result.back() == ',') result.pop_back(); // Remove last comma
        result += ";\n}";
        return result;
	}
}; // Removed normal
friend struct SMesh;
struct SMesh{ // Changed from typedef struct
	SMesh(CDnmConvX& c) : p_(c), mlist(c), pcnt(nullptr) {}; // Updated constructor, p to p_, init pcnt
	CDnmConvX& p_; // Changed to reference
	std::string name; // Qualified string
	std::vector<SVertex>vts;							// indexed vertex, Qualified vector, Changed vertex to SVertex
	std::vector<SFaceIdx>fcs;							// Indexed faces, Qualified vector, Changed faceIdx to SFaceIdx
	SMeshNormals normal; // Changed normal to SMeshNormals
	SMaterialList mlist; // Changed mlist to SMaterialList
	SVertex* pcnt = nullptr;								// reference to new mesh center, Changed vertex to SVertex, initialized
	// Removed SMesh():p(NULL),pcnt(NULL),mlist(*p){}
    [[nodiscard]] std::string operator std::string() const { // Now const
        if (vts.empty()) { // was vts.size() == 0
            return std::format("Mesh {} {{1;0;0;0;;1;3;0,0,0;;}}", name);
        }

        std::string result = std::format("Mesh {} {{\n{}", name, vts.size());
        result += ";";

        if (pcnt && pcnt->anyChange()) {
            for (const auto& v_orig : vts) {
                SVertex temp_v = v_orig - (*pcnt);
                result += std::format("\n{}", temp_v.operator std::string());
                result += ",";
            }
        } else {
            for (const auto& v_orig : vts) {
                result += std::format("\n{}", v_orig.operator std::string());
                result += ",";
            }
        }
        if (!vts.empty() && result.back() == ',') result.pop_back();
        result += ";\n";

        result += std::format("{};", fcs.size());
        for (const auto& face_val : fcs) {
            result += std::format("\n{},", face_val.operator std::string());
        }
        if (!fcs.empty() && result.back() == ',') result.pop_back();
        result += ";\n";

        result += mlist.operator std::string() + "\n";

        // REMOVED: checkNormal(name); - must be called externally if normals need update.
        result += normal.operator std::string() + "\n}";
        return result;
    }
	void checkNormal(const std::string&name){ // Qualified string
		if(vts.size()==normal.vts.size())return;
		SVertex v_init={0,0,0,false};					// initializer vertex, Changed vertex to SVertex, renamed v to v_init
		std::vector<SVertex>n_vec(vts.size(),v_init);			// random access of normal vertex, Qualified vector, Changed vertex to SVertex, Renamed n to n_vec
		auto normal_vts_it = normal.vts.begin(); // Changed itvV it to auto normal_vts_it
		for(const auto& face_val : fcs){						// iterate each 144 faces, Replaced each itvFI
			for(const auto& v_idx : face_val.vfi)				// and each 4 or 3 vertex in face, Replaced each it_vu_
				n_vec[v_idx]+=(*normal_vts_it);				// sum normal vertex to n[vertIdx]
			++normal_vts_it;								// next normal vertex
		}
		u16 idx=-1;
		for(auto& vertex_val : n_vec)							// after sum all normals, Replaced each itvV
			vertex_val.normalize(name,++idx);		// normalize each
		normal.vts.swap(n_vec);						// save the normalized group
		return;
	}
	std::vector<u16>listFaceIdx(const std::string&mt){ // Qualified vector, string
		std::vector<u16>r_vec; // Qualified vector, Renamed r to r_vec
		u16 i_idx=0;								// count faces index, Renamed i to i_idx
		for(const auto& val : mlist.mtIdx){				// mtIdx.size=fcs.size, Replaced each it_vs
			if(val==mt)r_vec.push_back(i_idx);		// true on range 22~44 46~49
			i_idx++;
		}										// r.size 25 faces
		return r_vec;
	}
	std::vector<u16>listVertIdx(const std::vector<u16>&fcid){ // Qualified vector
		std::map<u16,u16>vfMap;						// store vertex idx and its face count, Qualified map
		// u16 i=0; // i not used
		for(const auto& face_idx_val : fcid){						// each face index, Replaced each it_vu_
			for(const auto& vert_idx_val : fcs[face_idx_val].vfi){		// each vertex index in face, Replaced each it_vu
				auto it_map = vfMap.find(vert_idx_val);	// find vertex before saving, Renamed it to it_map
				if(it_map==vfMap.end())				// true if new vertex index
					vfMap[vert_idx_val]=1;			// initialize
				else ++vfMap[vert_idx_val];			// else increase face count
			}
		}
		std::vector<u16>r_vec; // Qualified vector, Renamed r to r_vec
		for(const auto& pair_ : vfMap){ // Replaced each it_uu
			r_vec.push_back(pair_.first);
		}
		return r_vec;
	}
	std::vector<u16>listUsedVertIdx(){ // Qualified vector
		std::vector<u16>u_vec(vts.size(),0);				// store how many face use each vertex, Qualified vector, Renamed u to u_vec
		for(const auto& face_val : fcs){						// each face index, Replaced each itvFI
			for(const auto& v_idx : face_val.vfi){			// each vertex index in face, Replaced each it_vu_
				++u_vec[v_idx];					// increase face count
			}
		}
		return u_vec;
	}
	void invertFace(u16 idx){
		fcs[idx].reverseContent();
		normal.invertFace(idx);
	}
	void invertFace(std::string mt){ // Qualified string
		checkNormal(name);
		std::map<u16,SVertex>submh; // Qualified map, Changed vertex to SVertex
		SMesh mh_local; // Changed mesh to SMesh, Renamed mh to mh_local
		auto itf_it = fcs.begin(); // Renamed itf to itf_it
		auto itn_it = normal.fcs.begin(); // Renamed itn to itn_it
		auto itv_it = vts.begin(); // Renamed itv to itv_it
		for(const auto& mat_name : mlist.mtIdx){			// each string in material index, range 22~44 46~49, Replaced each it_vs_
			if(mat_name==mt){						// true on range 22~44 46~49
				itf_it->reverseContent();				// reverse face vertex index
				itn_it->reverseContent();				// reverse normal vertex index
				// need map<vertIdx,vertCoord> of each vert in itf
				mh_local.fcs.push_back(*itf_it);				// face index
				mh_local.vts.push_back(*itv_it);				// vertex coords
				// normal face index, vertex coords
				auto it_map = mlist.mtMap.find(mt);		// get material, Renamed it to it_map
				if(it_map!=mlist.mtMap.end())
					mh_local.mlist.mtMap[mt]=it_map->second;	// copy material
			}
			++itf_it;++itn_it; ++itv_it;							// next face, next normal
		}
	}
	SMesh&clear(){
		name.clear();
		vts.clear();
		fcs.clear();
		normal.vts.clear();
		normal.fcs.clear();
		mlist.mtIdx.clear();
		mlist.mtMap.clear();
		pcnt=NULL;
		return*this;
	}
}; // Removed mesh
struct SQuaternion{ // Changed from typedef struct
	f32 w,x,y,z;
	SQuaternion&rad(f32 f){
		const f32 hp(1.5707963f);				// pi/2
		f32 r=hp*f,s;
		s=std::sinf(r); // Qualified sinf
		x*=s;y*=s;z*=s;
		w=std::cosf(r); // Qualified cosf
//		w=std::sqrtf(1-x*x-y*y-z*z);					// alternative calc, Qualified sqrtf
		return*this;
	}
	SQuaternion&ri16(i32 i){					// valid -32786 up to 32768
//		const f32 p=20860.756f;					// 65536/pi to 32bits equivalent
		const f32 r_val=i/20860.756f; // Renamed r to r_val
		f32 s=std::sinf(r_val); // Qualified sinf
		x*=s;y*=s;z*=s;
		w=std::cosf(r_val); // Qualified cosf
//		w=std::sqrtf(1-x*x-y*y-z*z);					// alternative calc, Qualified sqrtf
		if(i>32768||i<-32768)w=-w;				// i16 lack of positive 32768 val
		return*this;
	}
	af9 getMatrix(){
		af9 m;
		m[0]=1-2*y*y-2*z*z;m[1]=2*x*y-2*w*z;m[2]=2*x*z+2*w*y;
		m[3]=2*x*y+2*w*z;m[4]=1-2*x*x-2*z*z;m[5]=2*y*z-2*w*x;
		m[6]=2*x*z-2*w*y;m[7]=2*y*z+2*w*x;m[8]=1-2*x*x-2*y*y;
		return m;
	}
	af3 rotateVert(SVertex&v){ // Changed vertex to SVertex
		SVertex q_vert={x,y,z,false},r_vert; // Changed vertex to SVertex, Renamed q to q_vert, r to r_vert
		r_vert=v+(q_vert*2).cross(q_vert.cross(v)+(v*w));
		af3 a_val={r_vert.x,r_vert.y,r_vert.z}; // Renamed a to a_val
		return a_val;
	}
	af3 rotateVert(const af3&c){
		SVertex v={c[0],c[1],c[2],false}; // Changed vertex to SVertex
		return rotateVert(v);
	}
	SQuaternion&operator=(const SVertex&v){ // Changed vertex to SVertex
		w=0;
		x=v.x;y=v.y;z=v.z;
		return*this;
	}
	SQuaternion&operator=(const SQuaternion&q){
		w=q.w;x=q.x;y=q.y;z=q.z;
		return*this;
	}
	SQuaternion operator*(const SQuaternion&q){
		SQuaternion r_val; // Renamed r to r_val
		r_val.w=(w*q.w-x*q.x-y*q.y-z*q.z);
		r_val.x=(w*q.x+x*q.w+y*q.z-z*q.y);
		r_val.y=(w*q.y-x*q.z+y*q.w+z*q.x);
		r_val.z=(w*q.z+x*q.y-y*q.x+z*q.w);
		return r_val;
	}
	SQuaternion&operator*=(const SQuaternion&q){
		const f32 r_arr[4]= // Renamed r to r_arr
			{w*q.w-x*q.x-y*q.y-z*q.z
			,x*q.w+w*q.x-z*q.y+y*q.z
			,y*q.w+z*q.x+w*q.y-x*q.z
			,z*q.w-y*q.x+x*q.y+w*q.z};
		w=r_arr[0];x=r_arr[1];y=r_arr[2];z=r_arr[3];
		return*this;
	}
	SQuaternion&normalize(){
		f32 f=w*w+x*x+y*y+z*z,x2,y2,z2;
		i32 i_val=static_cast<i32>(1000000*f); // Renamed i to i_val
		if(i_val!=1000000){
			f=std::sqrtf(f); // Qualified sqrtf
			x/=f;y/=f;z/=f;
			x2=x*x;y2=y*y;z2=z*z;
			w=std::sqrtf(1-x2-y2-z2); // Qualified sqrtf
		}
		return*this;
	}
	SQuaternion&ai16(const ai3&a){
		SQuaternion t={0,0,1,0},p={0,1,0,0},b={0,0,0,1};
		t.ri16(a[0]);							// turn west/east
		p.ri16(a[1]);							// pitch up/down
		b.ri16(a[2]);							// bank left/right (inverted)
		(*this)=(b*p)*t;
		return*this;
	}
    [[nodiscard]] operator std::string() const {
        return std::format("4;{:.6f},{:.6f},{:.6f},{:.6f};;", w, x, y, z);
    }
}; // Removed quat
struct STransform{ // Changed from typedef struct
	af9 a;										// angle of rotation
	af3 c;										// center of translation
	STransform&operator=(const STransform&t){
		a=t.a;
		c=t.c;
		return*this;
	}
	STransform&operator=(const af3&p_val){ // Renamed p to p_val
		c=p_val;
		return*this;
	}
	STransform&operator=(const SVertex&p_val){ // Changed vertex to SVertex, Renamed p to p_val
		c[0]=p_val.x;c[1]=p_val.y;c[2]=p_val.z;
		return*this;
	}
	STransform&operator=(const af9&m_val){ // Renamed m to m_val
		a=m_val;
		return*this;
	}
	STransform&operator=(const ai3&i_val){ // Renamed i to i_val
		const f32 p_const=10430.378350470453f;		// 32768/pi, Renamed p to p_const
		const bool keepCenter=true;
		STransform m_trans; // Renamed m to m_trans
		if(i_val[2]){								// true if bank angle
			f32 z=i_val[2]/p_const;						// bank left/right (left handed)
			a[0]=std::cosf(z);a[1]=std::sinf(z); // Qualified cosf, sinf
			a[3]=-std::sinf(z);a[4]=std::cosf(z); // Qualified sinf, cosf
			if(i_val[1]){							// true if bank and pith angle
				m_trans.reset(keepCenter);
				f32 x=i_val[1]/p_const;					// pith up/down (right handed)
				m_trans.a[4]=std::cosf(x);m_trans.a[5]=-std::sinf(x); // Qualified cosf, sinf
				m_trans.a[7]=std::sinf(x);m_trans.a[8]=std::cosf(x); // Qualified sinf, cosf
				(*this)*=m_trans;						// aggregated
			}
			if(i_val[0]){							// true if bank, pitch and turn angle
				m_trans.reset(keepCenter);
				f32 y=i_val[0]/p_const;					// turn around west/east (right handed)
				m_trans.a[0]=std::cosf(y);m_trans.a[2]=std::sinf(y); // Qualified cosf, sinf
				m_trans.a[6]=-std::sinf(y);m_trans.a[8]=std::cosf(y); // Qualified sinf, cosf
				(*this)*=m_trans;						// aggregated
			}
		}else if(i_val[1]){							// true if pitch angle
				f32 x=i_val[1]/p_const;					// bank left/right (right handed)
				a[4]=std::cosf(x);a[5]=-std::sinf(x); // Qualified cosf, sinf
				a[7]=std::sinf(x);a[8]=std::cosf(x); // Qualified sinf, cosf
				if(i_val[0]){						//true if pitch and turn angle
					m_trans.reset(keepCenter);
					f32 y=i_val[0]/p_const;				// turn around west/east (right handed)
					m_trans.a[0]=std::cosf(y);m_trans.a[2]=std::sinf(y); // Qualified cosf, sinf
					m_trans.a[6]=-std::sinf(y);m_trans.a[8]=std::cosf(y); // Qualified sinf, cosf
					(*this)*=m_trans;					// aggregated
				}
		}else if(i_val[0]){							// true if turn angle only
			f32 y=i_val[0]/p_const;						// turn around west/east (right handed)
			a[0]=std::cosf(y);a[2]=std::sinf(y); // Qualified cosf, sinf
			a[6]=-std::sinf(y);a[8]=std::cosf(y); // Qualified sinf, cosf
		}
		return*this;
	}
	STransform&operator=(SQuaternion q){ // Changed quat to SQuaternion
		a=q.getMatrix();
//		c=q.rotateVert(c);
		return*this;
	}
	STransform operator*(STransform t){
		STransform r_val; // Renamed r to r_val
		r_val.a[0]=a[0]*t.a[0]+a[1]*t.a[3]+a[2]*t.a[6];
		r_val.a[1]=a[0]*t.a[1]+a[1]*t.a[4]+a[2]*t.a[7];
		r_val.a[2]=a[0]*t.a[2]+a[1]*t.a[5]+a[2]*t.a[8];
		r_val.a[3]=a[3]*t.a[0]+a[4]*t.a[3]+a[5]*t.a[6];
		r_val.a[4]=a[3]*t.a[1]+a[4]*t.a[4]+a[5]*t.a[7];
		r_val.a[5]=a[3]*t.a[2]+a[4]*t.a[5]+a[5]*t.a[8];
		r_val.a[6]=a[6]*t.a[0]+a[7]*t.a[3]+a[8]*t.a[6];
		r_val.a[7]=a[6]*t.a[1]+a[7]*t.a[4]+a[8]*t.a[7];
		r_val.a[8]=a[6]*t.a[2]+a[7]*t.a[5]+a[8]*t.a[8];
		r_val.c=this->c;
		return r_val;
	}
	STransform&operator*=(const STransform&t){
		(*this)=(*this)*t;
		return*this;
	}
// 	af3 invertCnt(){
// 		af3 r={-c[0],-c[1],-c[2]};
// 		return r;
// 	}
	bool anyChange(){
		return (c[0]||c[1]||c[2]||a[1]||a[2]||a[5]);
	}
	void reset(bool keepCenter=false){
		af9 t={1,0,0,0,1,0,0,0,1};
		a.swap(t);								// reset rotation
		if(!keepCenter)
			c.assign(0);						// reset center position
	}
    [[nodiscard]] operator std::string() const {
        return std::format("FrameTransformMatrix{{\n"
                           "{:.6f},{:.6f},{:.6f},0,\n"
                           "{:.6f},{:.6f},{:.6f},0,\n"
                           "{:.6f},{:.6f},{:.6f},0,\n"
                           "{:.6f},{:.6f},{:.6f},1.0;;\n}}",
                           a[0], a[1], a[2],
                           a[3], a[4], a[5],
                           a[6], a[7], a[8],
                           c[0], c[1], c[2]);
    }
}; // Removed mtx
using ituQ = std::map<u16,SQuaternion>::iterator; // Qualified map, Changed quat to SQuaternion
using ituV = std::map<u16,SVertex>::iterator; // Qualified map, Changed vertex to SVertex
using ituuu = std::map<u16,std::map<u16,u16>>::iterator; // Qualified map
friend struct SAnimationKey;
struct SAnimationKey{					// Animation{ // Changed from typedef struct
	SAnimationKey(CDnmConvX& c) : p_(c) {}; // Updated constructor, p to p_
	CDnmConvX& p_; // Changed to reference
	std::string name;								// name of frame to animate, Qualified string
	std::map<u16,SQuaternion>agMap;							// map of keys and angles, Qualified map, Changed quat to SQuaternion
	std::map<u16,SVertex>mvMap;						// map of keys and movement, Qualified map, Changed vertex to SVertex
	std::vector<std::string>parents;						// list of frames to merge with, Qualified vector, string
	SVertex c;									// new offset center, Changed vertex to SVertex
	u16 cla;									// type of anim
	std::vector<af3>poss;							// world position coordinates, Qualified vector
	std::vector<ai3>tpbs;							// 3 angles -32768 up to 32768(65536+1), Qualified vector
	std::vector<bool>disps;							// visible status at animation state coord, Qualified vector
	// Removed SAnimationKey():p(NULL){};
	void clear(){
		name.clear();
		agMap.clear();
		mvMap.clear();
		parents.clear();
		c.clear();
		cla=0;
		poss.clear();
		tpbs.clear();
		disps.clear();
	}
	void calcSelf(){
		SVertex o; // Changed vertex to SVertex
		ai3&a=p_.frs.frMap[name].tpb;			// shortcut to current frame angle, p to p_
		SQuaternion q_val=SQuaternion().ai16(a);	// rotate using current frame angle, Changed quat to SQuaternion, Renamed q to q_val
		if(&p_ != nullptr && p_.otl.size()&&tpbs.size()){		// true when there is pose config, p to p_
			for(const auto& otl_pair : p_.otl){					// each required keyframe from config, Replaced each ituuu, p to p_
				const u16&k=otl_pair.first;		// shortcut to current keyframe from config
				u16&i_val=otl_pair.second.at(cla);		// shortcut to cla status from config, Renamed i to i_val, used .at() for map
				o=poss[i_val];						// convert new position coords to vertex
				mvMap[k]=o+c;					// output position animkey and frame origin
				ai3&b_val=tpbs[i_val];					// shortcut to current animkey angles, Renamed b to b_val
				SQuaternion r_quat=SQuaternion().ai16(b_val);				// turn, pitch and bank animkey, Changed quat to SQuaternion, Renamed r_val to r_quat
				agMap[k]=r_quat*q_val;					// output animkey and frame orientation
			}
		}else{									// output all posible animkey sorted by cla
			cla*=10;
			for(u16 i_idx=0;i_idx<tpbs.size();++i_idx){ // Renamed i to i_idx
				ai3&b_val=tpbs[i_idx];					// shortcut to current animkey angles, Renamed b to b_val
				SQuaternion r_quat=SQuaternion().ai16(b_val);				// turn, pitch and bank animkey, Changed quat to SQuaternion, Renamed r_val to r_quat
				agMap[cla+i_idx]=r_quat*q_val;				// output animkey and frame orientation
				o=poss[i_idx];						// get new position coords
				mvMap[cla+i_idx]=o+c;				// update animkey position
			}
		}
	}
	void calcParent(const std::string&s=""){ // Qualified string
		if(s==""|| &p_ == nullptr)return; // p to p_
		SAnimationKey&a_key=p_.frs.frMap[s].ak; // Renamed a to a_key, p to p_
		a_key.calcSelf();
		for(const auto& otl_pair : p_.otl){ // Replaced each ituuu, p to p_
			const u16&k=otl_pair.first;
			SQuaternion&r_quat=agMap[k]; // Changed quat to SQuaternion, Renamed r_val to r_quat
			r_quat=r_quat*a_key.agMap[k];
//			SVertex&o=mvMap[k]; // Changed vertex to SVertex
//			o=o+mvMap[k];
		}
	}
    [[nodiscard]] operator std::string() const { // Now const
        // REMOVED: calcSelf();
        // REMOVED: loop with calcParent(*it_vs_);
        // These must be called externally if data needs recalculation.

        std::string result = "AnimationKey{0;\n";
        result += std::format("{};\n", agMap.size());

        std::string agMap_str;
        for (const auto& pair_ : agMap) {
            agMap_str += std::format("{};{},\n", pair_.first, pair_.second.operator std::string());
        }
        if (!agMap_str.empty()) {
            // Remove trailing ",\n"
            agMap_str.pop_back(); // removes \n
            agMap_str.pop_back(); // removes ,
        }
        result += agMap_str;
        if (!agMap.empty()) result += "\n"; // Add newline if there were entries
        result += ";\n"; // Terminator for agMap entries
        result += "}\nAnimationKey{2;\n";

        result += std::format("{};\n", mvMap.size());
        std::string mvMap_str;
        for (const auto& pair_ : mvMap) {
            mvMap_str += std::format("{};3;{:.6f},{:.6f},{:.6f};;,\n",
                                     pair_.first,
                                     pair_.second.x, pair_.second.y, pair_.second.z);
        }
        if (!mvMap_str.empty()) {
            // Remove trailing ",\n"
            mvMap_str.pop_back(); // removes \n
            mvMap_str.pop_back(); // removes ,
        }
        result += mvMap_str;
        if(!mvMap.empty()) result += "\n"; // Add newline if there were entries
        result += ";\n"; // Terminator for mvMap entries
        result += "}";
        return result;
    }
}; // Removed anikey
friend struct SFrame;
struct SFrame{ // Changed from typedef struct
	SFrame(CDnmConvX& c) : p_(c), nested(false), ak(c) { ftm.reset(); }; // Updated constructor, p to p_, ak(*p) to ak(c)
	CDnmConvX& p_; // Changed to reference
	bool nested;
	std::string name; // Qualified string
	STransform ftm;									// FrameTransformMatrix, Changed mtx to STransform
	SVertex cnt;									// new center for the nested mesh, Changed vertex to SVertex
	af3 pos;									// world position coordinates
	ai3 tpb;									// 3 angles -32768 up to 32768(65536+1)
	bool disp;									// visible status at still position coord
	std::string mhId;								// nested mesh, Qualified string
	SAnimationKey ak;									// animation of the frame, Changed anikey to SAnimationKey
	std::vector<std::string>frIds;						// id of nested frames, Qualified vector, string
	// Removed SFrame():p(NULL),nested(false),ak(*p){ftm.reset();}

    void update_transform_and_animation_center() {
        for (std::uint8_t i = 0; i < 3; ++i) {
            ftm.c[i] += pos[i]; // ftm is STransform, pos is af3
        }
        SQuaternion q = {1.0f, 0.0f, 0.0f, 0.0f}; // Ensure SQuaternion can be initialized like this
        ftm = q.ai16(tpb); // tpb is ai3. STransform::operator=(SQuaternion) and SQuaternion::ai16(ai3) are used.
        // Assuming SVertex has operator=(const af3&) which it does.
        ak.c = ftm.c;      // ak is SAnimationKey. SAnimationKey::c is SVertex. STransform::c is af3.
    }

	SFrame&operator=(const SFrame&f){			// nested should not copy
		// p=f.p; // Should not copy reference if it's to the same CDnmConvX instance. If frames can be moved between CDnmConvX instances, this needs more thought. For now, assume p_ is set at construction.
//		nested=f.nested;
		name=f.name;
 		ftm=f.ftm;
		cnt=f.cnt;
		pos=f.pos;
		tpb=f.tpb;
		disp=f.disp;
		mhId=f.mhId;
		ak=f.ak;
		frIds=f.frIds;
		return*this;
	}
	void clear(){
		nested=false;
		name.clear();
		ftm.reset();
		cnt=SVertex{0}; // Changed vertex to SVertex
		pos.fill(0.0f); // af3 is std::array<f32,3>
		tpb.fill(0);    // ai3 is std::array<i32,3>
		disp=false;
		mhId.clear();
		ak.clear();
		frIds.clear();
	}

    operator std::string() const { // new const version
        if (name.empty()) return "{}";

        std::string result;
        result += std::format("Frame {} {{\n", name);

        if (ftm.anyChange()) {
            result += ftm.operator std::string() + "\n";
        }

        if (!mhId.empty() && mhId != "null") {
             result += std::format("{{{{{}}}}}\n", mhId); // Escape braces for format, then for X file
        }

        for (const auto& child_frId : frIds) {
            if (!child_frId.empty()) {
                result += std::format("{{{{{}}}}}\n", child_frId);
            }
        }

        result += ak.operator std::string() + "\n";

        result += "}";
        return result;
    }
}; // Removed frame
using itsMT_ = std::map<std::string,SMaterial>::const_iterator; // Qualified map, string, Changed material to SMaterial
struct SMapCollMat{ // Changed from typedef struct
	std::map<std::string,SMaterial>mtMap; // Qualified map, string, Changed material to SMaterial
	SMapCollMat&operator<<(const SMaterial&m){ // Changed material to SMaterial
		if(m.name!="")mtMap[m.name]=m;
		return*this;
	}
	SMapCollMat&operator>>(SMaterial&m){ // Changed material to SMaterial
		itsMT it=mtMap.begin(); // itsMT is std::map<std::string,SMaterial>::iterator
		m=it->second;
		mtMap.erase(it);
		return*this;
	}
	SMapCollMat&operator<<(const SMaterialList&m){ // Changed mlist to SMaterialList
		for(const auto& pair_ : m.mtMap) // Changed each macro to range-based for, Renamed pair to pair_
			mtMap[pair_.first]=pair_.second;
		return*this;
	}
	SMapCollMat&operator>>(SMaterialList&m){ // Changed mlist to SMaterialList
		m.mtMap.swap(mtMap); // Use swap for efficiency
		mtMap.clear(); // Ensure original is empty
		return*this;
	}
	[[nodiscard]] std::string operator std::string() const { // Renamed cstr to std::string
		std::string result;
		for (const auto& pair_ : mtMap) {
			result += pair_.second.operator std::string() + "\n";
		}
		result += "\n"; // Corrected: Append newline to result
		return result;  // Corrected: Return result
	}
}; // Removed collMat
using itsMH = std::map<std::string,SMesh>::iterator; // Qualified map, string, Changed mesh to SMesh
friend struct SMapCollMsh;
struct SMapCollMsh{ // Changed from typedef struct
	SMapCollMsh(CDnmConvX& c) : p_(c) {}; // p to p_
	CDnmConvX& p_; // p to p_
	std::map<std::string,SMesh>mhMap; // Qualified map, string, Changed mesh to SMesh
	SMapCollMsh&operator<<(const SMesh&m){ // Changed mesh to SMesh
		if(m.name!="")mhMap[m.name]=m;
		return*this;
	}
	SMapCollMsh&operator>>(SMesh&m){ // Changed mesh to SMesh
		itsMH it=mhMap.begin();
		m=it->second;
		mhMap.erase(it);
		return*this;
	}
    [[nodiscard]] operator std::string() const { // new
        // REMOVED: Blacklist processing logic that modifies mhMap
        std::string result;
        for (const auto& pair_ : mhMap) { // mhMap is std::map<std::string, SMesh>
            if (!pair_.first.empty()) { // Original was itsMH->first!=""
                result += pair_.second.operator std::string() + "\n";
            }
        }
        result += "\n";
        return result;
    }
}; // Removed collMsh
using itsFR = std::map<std::string,SFrame>::iterator; // Qualified map, string, Changed frame to SFrame
friend struct SMapCollFrm;
struct SMapCollFrm{ // Changed from typedef struct
	SMapCollFrm(CDnmConvX& c) : p_(c) {}; // p to p_
	CDnmConvX& p_; // p to p_
	std::map<std::string,SFrame>frMap; // Qualified map, string, Changed frame to SFrame
	SMapCollFrm&operator<<(const SFrame&f){ // Changed frame to SFrame
		if(f.name!=""){
			SFrame&fr=frMap[f.name];				// find where to save, Changed frame to SFrame
			fr=f;								// save the frame
		}
		return*this;
	}
	SMapCollFrm&operator>>(SFrame&f){ // Changed frame to SFrame
		itsFR it=frMap.begin();
		f=it->second;
		frMap.erase(it);
		return*this;
	}
    [[nodiscard]] operator std::string() const {
        std::string result;
        // The original logic iterated to find main parent frames.
        // This SMapCollFrm operator will format its top-level, non-nested frames.
        // SFrame::operator std::string() is now const and formats a single frame
        // including its direct children's IDs and its own animation key.
        for (const auto& pair_ : frMap) { // frMap is std::map<std::string, SFrame>
            if (!pair_.second.nested && !pair_.second.name.empty()) {
                result += pair_.second.operator std::string() + "\n";
            }
        }
        result += "\n"; // Original had an extra endl after the loop
        return result;
    }
}; // Removed collFrm
using itsA = std::map<std::string,SAnimationKey>::iterator; // Qualified map, string, Changed anikey to SAnimationKey
friend struct SMapCollAnim;
struct SMapCollAnim{ // Changed from typedef struct
	SMapCollAnim(CDnmConvX& c) : p_(c) {}; // p to p_
	CDnmConvX& p_; // p to p_
	std::map<std::string,SAnimationKey>akMap;					// AnimationSet{, Qualified map, string, Changed anikey to SAnimationKey
	SMapCollAnim&operator<<(SAnimationKey&ak){ // Changed anikey to SAnimationKey
		akMap[ak.name]=ak;
		return*this;
	}
    [[nodiscard]] operator std::string() const { // new
        std::string result = "AnimationSet{\n";
        for (const auto& pair_ : akMap) { // akMap is std::map<std::string, SAnimationKey>
            if (!pair_.second.name.empty()) { // was itsA->second.name!=""
                result += std::format("Animation{{\n{{}}\n" // placeholder for pair_.first (frame name)
                                    "{}\n" // SAnimationKey string
                                    "AnimationOptions{{0;0;}}\n}}\n",
                                    pair_.first,
                                    pair_.second.operator std::string());
            }
        }
        result += "}\n";
        return result;
    }
}; // Removed collAni

private:
	std::string inFilePath; // Qualified string
	SMapCollMat mts,omts;							// material collector and output, Changed collMat to SMapCollMat
	SMapCollMsh mhs,omhs;							// mesh collector and output, Changed collMsh to SMapCollMsh
	SMapCollFrm frs;								// frame collector, Changed collFrm to SMapCollFrm
	SMapCollAnim aks;								// animkey collector, Changed collAni to SMapCollAnim
	bool nstmt;									// use nested material config
	bool nstmh;									// use nested mesh config
	bool mnm;									// merge frame/animkey of null mesh
	std::vector<std::string>configs;						// general config, Qualified vector, string
	std::vector<std::string>mhbl;							// blacklist mesh, Qualified vector, string
	std::vector<std::string>frbl;							// blacklist frame, Qualified vector, string
	std::vector<std::string>dsmh;							// double side mesh, Qualified vector, string
	std::vector<std::string>invfidx;						// invert face by idx, Qualified vector, string
	std::vector<std::string>invfmt;						// invert face by material, Qualified vector, string
	std::map<u16,std::map<u16,u16>>otl;					// keyframes<cla,sta> relationship, Qualified map
};

// manipulator to skip any char - REMOVED as unused
// template<char C>
// std::istream&skip(std::istream&is){
// 	if((is>>std::ws).peek()==C)is.ignore(); // std::ws is already qualified
// 	else is.setstate(std::ios_base::failbit); // std::ios_base is already qualified
// 	return is;
// }// i.e.  istr>>skip<'#'>;
// #pragma warning directives removed
#endif // DNMCONVX_H_
