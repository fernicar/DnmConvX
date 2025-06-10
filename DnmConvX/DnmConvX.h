#ifndef DNMCONVX_H_
#define DNMCONVX_H_
#include <array>
#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <map>
#include <cmath>
#include <cstdint>
#include <functional>
#include <regex>
#include <format>
#include <span>

// Basic type typedefs removed

// using aliases for complex types
/** @brief Alias for a 3D integer array, typically for indices or integer coordinates. */
using ai3 = std::array<std::int32_t,3>;
/** @brief Alias for a 3D float array, typically for coordinates or colors. */
using af3 = std::array<float,3>;
/** @brief Alias for a 9-element float array, typically for a 3x3 matrix. */
using af9 = std::array<float,9>;
/** @brief Alias for a vector of strings. */
using vs = std::vector<std::string>;
/** @brief Alias for a vector of 32-bit integers. */
using vi = std::vector<std::int32_t>;
/** @brief Alias for a vector of 16-bit unsigned integers. */
using vu = std::vector<std::uint16_t>;
/** @brief Alias for a vector of floats. */
using vf = std::vector<float>;
/** @brief Alias for a map with uint16_t keys and values, typically for index mappings. */
using uu = std::map<std::uint16_t,std::uint16_t>;

// using aliases for iterators
/** @brief Const iterator for a vector of strings. */
using it_vs_ = vs::const_iterator;
/** @brief Iterator for a vector of strings. */
using it_vs = vs::iterator;
/** @brief Const iterator for a vector of 32-bit integers. */
using it_vi_ = vi::const_iterator;
/** @brief Iterator for a vector of 32-bit integers. */
using it_vi = vi::iterator;
/** @brief Const iterator for a vector of 16-bit unsigned integers. */
using it_vu_ = vu::const_iterator;
/** @brief Iterator for a vector of 16-bit unsigned integers. */
using it_vu = vu::iterator;
/** @brief Const iterator for a vector of floats. */
using it_vf_ = vf::const_iterator;
/** @brief Iterator for a vector of floats. */
using it_vf = vf::iterator;
/** @brief Const iterator for a map of uint16_t to uint16_t. */
using it_uu_ = uu::const_iterator;
/** @brief Iterator for a map of uint16_t to uint16_t. */
using it_uu = uu::iterator;

/** @brief Enumerates possible error codes for file operations and parsing stages. */
enum E_ERROR {
	E_NotError,      ///< Operation completed successfully without any errors.
	E_IniPath,       ///< An invalid path was provided for an INI configuration file.
	E_IniRead,       ///< Failed to read an INI configuration file (e.g., file does not exist, insufficient permissions).
	E_MemoryError,   ///< A memory allocation attempt failed during processing.
	E_DnmRead,       ///< Failed to read a DNM (Dynamodel) file (e.g., file does not exist, permissions, corrupted format).
	E_XWrite,        ///< Failed to write the output .x (DirectX) file (e.g., path invalid, insufficient permissions).
	E_Error          ///< A generic or unspecified error occurred during the conversion process.
};

/** @brief A boolean-like enum, typically for flags within parsing logic to indicate a binary state. */
enum class E_flib {NOT=false, YES=true};
/** @brief Prefix increment operator for E_flib. Sets to YES. */
inline E_flib& operator++(E_flib&b) { b=E_flib::YES; return b; }
/** @brief Postfix increment operator for E_flib. Sets to YES. */
inline E_flib operator++(E_flib&b,int) { E_flib t=b; ++b; return t; }
/** @brief Prefix decrement operator for E_flib. Sets to NOT. */
inline E_flib& operator--(E_flib&b) { b=E_flib::NOT; return b; }
/** @brief Postfix decrement operator for E_flib. Sets to NOT. */
inline E_flib operator--(E_flib&b,int) { E_flib t=b; --b; return t; }

/**
 * @class CDnmConvX
 * @brief Main class for converting YSFlight Dynamodel (.dnm) files to DirectX (.x) format.
 * It handles parsing of DNM files, applying transformations and configurations specified
 * in an INI file, and outputting the result as an X file.
 */
class CDnmConvX{
public:
	/** @brief Default constructor. Initializes members to default states, including collections and flags. */
	CDnmConvX(void) : mts(*this), omts(*this), mhs(*this), omhs(*this), frs(*this), aks(*this), nstmt(false), nstmh(false), mnm(false), inFilePath("") {}
// 	~CDnmConvX(void); // Destructor might be needed if manual resource management occurs (e.g. raw pointers). Currently not the case.

	/** @brief Processes an input string stream, typically for parsing lines from an INI configuration file.
	 *  @param iss The input string stream to process.
	 *  @return Reference to this CDnmConvX object. */
	CDnmConvX&operator<<(std::istringstream&iss);

	/** @brief Converts the entire loaded and processed model data into a single string formatted for a DirectX .x file.
	 *  @return A string containing the .x file content. */
	[[nodiscard]] operator std::string() const;

	/** @brief Finalizes data structures after all input files (DNM, INI) are parsed.
	 * This may involve applying transformations, resolving references, or preparing data for output. */
	void finalizeData();

	/** @brief Outputs the converted model to a DirectX .x file.
	 *  @param outPath The file system path where the .x file will be written. If empty, a default name might be generated based on the input file.
	 *  @return An E_ERROR code indicating success (E_NotError) or failure. */
	std::uint16_t outputToXFile(const char* outPath="");

	/** @brief Reads and parses a YSFlight Dynamodel (.dnm) file, populating internal data structures.
	 *  @param inPath Path to the .dnm file to be read.
	 *  @return An E_ERROR code indicating success (E_NotError) or failure. */
	std::uint16_t inputDnmFile(const char* inPath);

	/** @brief Reads and parses an INI configuration file.
	 *  The INI file provides settings that customize the DNM to X conversion process,
	 *  such as material adjustments, mesh blacklisting, or animation processing rules.
	 *  @param inPath Path to the .ini file.
	 *  @return An E_ERROR code indicating success (E_NotError) or failure. */
	std::uint16_t inputIniFile(const char* inPath);

private:
// Friend declarations allow these specialized collection structs to access private members of CDnmConvX.
// This design pattern is used here primarily to allow these structs to hold a reference (`p_`)
// back to the main CDnmConvX instance, enabling them to access global settings or other parts of the model
// during their operations (like string conversion or data manipulation).
friend struct SMaterialList;
friend struct SMapCollMsh;
friend struct SMapCollFrm;
friend struct SMapCollAnim;

/** @brief Represents a 15-bit color (RGB 5-5-5). Uses a union for easy initialization and bitfield access. */
struct UColor15Bit{
	union {
		std::uint16_t u;            ///< Combined 16-bit value for easy initialization/access.
		struct {
			std::uint16_t r:5;      ///< Red component (5 bits).
			std::uint16_t g:5;      ///< Green component (5 bits).
			std::uint16_t b:5;      ///< Blue component (5 bits).
			std::uint16_t p:1;      ///< Padding bit (unused).
		};
	};
	/** @brief Default constructor, initializes to black (all bits zero). */
	UColor15Bit() : u(0), r(0), g(0), b(0), p(0) {}
};
/** @brief Represents a 32-bit color with an alpha channel (RGBA 8-8-8-8). Uses a union for easy initialization and component access. */
struct UColor24Bit{ // Name implies 24-bit color, but structure is 32-bit RGBA
	union {
		std::uint32_t u;            ///< Combined 32-bit value for easy initialization/access.
		struct {
			std::uint8_t r;         ///< Red component (8 bits).
			std::uint8_t g;         ///< Green component (8 bits).
			std::uint8_t b;         ///< Blue component (8 bits).
			std::uint8_t a;         ///< Alpha component (8 bits).
		};
	};
	/** @brief Default constructor, initializes to transparent black (all bits zero). */
	UColor24Bit() : u(0), r(0), g(0), b(0), a(0) {}
	/** @brief Assigns from a 15-bit color, expanding components and defaulting alpha to opaque. */
	UColor24Bit&operator=(const UColor15Bit&c){
		r = static_cast<std::uint8_t>(c.r * 8 + c.r / 4); // Scale 5-bit to 8-bit
		g = static_cast<std::uint8_t>(c.g * 8 + c.g / 4);
		b = static_cast<std::uint8_t>(c.b * 8 + c.b / 4);
		a = 255; // Default to opaque when converting from 15-bit
		return*this;}
	[[nodiscard]] std::string operator std::string() const {
        return std::format("_{:02x}{:02x}{:02x}{:02x}_", r, g, b, a); // Added alpha
	}
};
/** @brief Represents a 3D vertex with position and a rounding flag. */
struct SVertex{
	float x = 0.0f; ///< X-coordinate.
	float y = 0.0f; ///< Y-coordinate.
	float z = 0.0f; ///< Z-coordinate.
	bool r = false; ///< Rounded flag (e.g., for smooth surface normal calculation).

	/** @brief Default constructor, initializes to origin (0,0,0) and not rounded. */
	SVertex() : x(0.0f), y(0.0f), z(0.0f), r(false) {}
	/** @brief Constructor from float array. */
	SVertex(const af3&c): x(c[0]),y(c[1]),z(c[2]),r(false){}
	/** @brief Constructor from individual coordinates. */
    SVertex(float vx, float vy, float vz, bool vr = false) : x(vx), y(vy), z(vz), r(vr) {}

	SVertex&operator=(const SVertex&v){
		x=v.x; y=v.y; z=v.z; r=v.r;
		return*this;
	}
	SVertex&operator=(const af3&c){
		x=c[0];y=c[1];z=c[2];r=false;
		return*this;
	}
	[[nodiscard]] SVertex operator-(const SVertex&v) const {
		SVertex a={x-v.x,y-v.y,z-v.z,false};
		return a;
	}
	[[nodiscard]] SVertex operator+(const SVertex&v) const {
		SVertex a={x+v.x,y+v.y,z+v.z,false};
		return a;
	}
	[[nodiscard]] std::string operator std::string() const {
        return std::format("{:.6f};{:.6f};{:.6f};", x, y, z);
	}
	[[nodiscard]] float dot(const SVertex&v) const {
		return (x*v.x+y*v.y+z*v.z);
	}
	[[nodiscard]] SVertex cross(const SVertex&v) const {
		SVertex r_val=	{y*v.z-z*v.y
					,z*v.x-x*v.z
					,x*v.y-y*v.x};
		return r_val;
	}
	[[nodiscard]] float length() const {return std::sqrtf(x*x+y*y+z*z);}
	void normalize(const std::string&name="",const std::uint16_t idx=0){
		float len=length(),mod=1;
		if(x==0&&y==0&&z==0)
			std::cerr<<name<<" V:"<<std::setw(4)<<std::right<<idx<<" invalid vertex normal\n";
		else mod=1/len;
		x*=mod;y*=mod;z*=mod;
		return;
	}
	void invert(){x=-x;y=-y;z=-z;}
	[[nodiscard]] bool anyChange() const {return x!=0||y!=0||z!=0;}
	void clear(){x=0;y=0;z=0;r=false;}
	SVertex&operator+=(const SVertex&v){
		x+=v.x;y+=v.y;z+=v.z;
		return*this;
	}
	[[nodiscard]] SVertex operator*(const float&f) const {
		SVertex n={x*f,y*f,z*f,false};
		return n;
	}
	[[nodiscard]] bool operator!=(const SVertex&v) const {return (x!=v.x||y!=v.y||z!=v.z);}
	[[nodiscard]] bool operator!=(const af3&v) const {return (x!=v[0]||y!=v[1]||z!=v[2]);}
};
using itvV = std::vector<SVertex>::iterator;
struct SFaceIdx{
	std::vector<std::uint16_t> vfi; ///< Stores vertex indices that form the face. Order matters for winding.

	/** @brief Default constructor. */
	SFaceIdx() = default;

	/** @brief Reverses the order of vertex indices in the face, effectively flipping its normal. */
	void reverseContent(){
		std::vector<std::uint16_t>r_val(vfi.rbegin(),vfi.rend());
		vfi.swap(r_val);
	}
	[[nodiscard]] std::string operator std::string() const {
        if (vfi.empty()) return "";
        std::string s = std::format("{};", vfi.size());
        for (std::size_t i = 0; i < vfi.size(); ++i) {
            s += std::format("{}", vfi[i]);
            if (i < vfi.size() - 1) {
                s += ",";
            }
        }
        s += ";";
        return s;
	}
};
using itvFI = std::vector<SFaceIdx>::iterator;

/** @brief Represents a color with three float components (RGB). */
struct SColor3Float{
	float r = 0.0f; ///< Red component.
	float g = 0.0f; ///< Green component.
	float b = 0.0f; ///< Blue component.

	/** @brief Default constructor, initializes to black. */
	SColor3Float() : r(0.0f), g(0.0f), b(0.0f) {}

	SColor3Float&operator=(const UColor24Bit&c24){
		r=c24.r/255.f;g=c24.g/255.f;b=c24.b/255.f;
		return*this;
	}
	SColor3Float&operator=(const SColor3Float&c){
		r=c.r;g=c.g;b=c.b;
		return*this;
	}
	SColor3Float&operator=(float f){
		r=g=b=f;
		return*this;
	}
	[[nodiscard]] std::string operator std::string() const {
        return std::format("{:.6f};{:.6f};{:.6f}", r, g, b);
	}
};

/** @brief Represents material properties. */
struct SMaterial{
	std::uint16_t i = 0;      ///< Material list index.
	std::string name;         ///< Name of the material.
	SColor3Float d;           ///< Diffuse color (RGB).
	float a = 1.0f;           ///< Alpha (opacity).
	float g = 0.0f;           ///< Glossiness (specular power).
	SColor3Float s;           ///< Specular color (RGB).
	SColor3Float e;           ///< Emissive color (RGB).

	/** @brief Default constructor. */
	SMaterial() : i(0), a(1.0f), g(32.0f) {} // Default gloss to a common value

	[[nodiscard]] std::string operator std::string() const {
        return std::format("Material {} {{\n"
                           "{};{:.6f};;\n"      // Diffuse color and alpha
                           "{:.6f};\n"          // Specular power (gloss)
                           "{};;\n"          // Specular color
                           "{};;\n}}",        // Emissive color
                           name, d.operator std::string(), a, g, s.operator std::string(), e.operator std::string());
	}
};
using itsMT = std::map<std::string,SMaterial>::iterator;

/** @brief Represents a list of materials for a mesh. */
struct SMaterialList{
	CDnmConvX& p_;                                  ///< Reference to the main converter object.
	std::vector<std::string> mtIdx;                 ///< Material names, indexed per face/vertex subset.
	std::map<std::string,SMaterial> mtMap;          ///< Map of material names to material properties.

	/** @brief Constructor. @param c Reference to the main converter. */
	SMaterialList(CDnmConvX& c) : p_(c) {};

	[[nodiscard]] std::string operator std::string() const {
        std::string result = std::format("MeshMaterialList {{\n"
                                     "{};\n"
                                     "{};\n",
                                     mtMap.size(), mtIdx.size());
        for(const auto& val : mtIdx) {
            auto it = mtMap.find(val);
            if (it != mtMap.end()) {
                result += std::format("{},", it->second.i);
            } else {
                result += "ERROR_IDX,";
            }
        }
        if (!mtIdx.empty() && result.back() == ',') result.pop_back();
        result += ";;\n";
        for (const auto& pair_ : mtMap) {
            if (p_.nstmt) {
                result += pair_.second.operator std::string() + "\n";
            } else {
                result += std::format("{{{}}}\n", pair_.second.name);
            }
        }
        result += "}";
        return result;
	}
};
struct SMeshNormals{
	std::vector<SVertex> vts;      ///< Vertex normals.
	std::vector<SFaceIdx> fcs;     ///< Face indices for normals (can differ from mesh face indices if normals are per-vertex-per-face).

	/** @brief Default constructor. */
	SMeshNormals() = default;

	void invertNormal(std::uint16_t idx){
		vts[idx].invert();
	}
	void invertFace(std::uint16_t idx){
		fcs[idx].reverseContent();
	}
	[[nodiscard]] std::string operator std::string() const {
        std::string result = std::format("MeshNormals{{\n{}", vts.size());
        result += ";";
        for (const auto& vertex_val : vts) {
            result += std::format("\n{},", vertex_val.operator std::string());
        }
        if (!vts.empty() && result.back() == ',') result.pop_back();
        result += ";\n";

        result += std::format("{};", fcs.size());
        for (const auto& face_idx_val : fcs) {
            result += std::format("\n{},", face_idx_val.operator std::string());
        }
        if (!fcs.empty() && result.back() == ',') result.pop_back();
        result += ";\n}";
        return result;
	}
};
friend struct SMesh;

/** @brief Represents a 3D mesh, including vertices, faces, normals, and materials. */
struct SMesh{
	CDnmConvX& p_;                                      ///< Reference to the main converter object, enabling access to global settings/data.
	std::string name;                                   ///< Name of the mesh.
	std::vector<SVertex> vts;                           ///< Collection of vertices forming the mesh geometry.
	std::vector<SFaceIdx> fcs;                          ///< Collection of faces, defined by indices into the `vts` vector.
	SMeshNormals normal;                                ///< Normals associated with the mesh, potentially per-vertex or per-face-vertex.
	SMaterialList mlist;                                ///< List of materials used by this mesh.
	// Non-owning pointer to an optional mesh center vertex. Its lifetime must be managed externally.
	SVertex* pcnt;                                      ///< Optional non-owning pointer to a mesh center vertex, used for repositioning.

	/** @brief Constructor. @param c Reference to the main converter. Initializes members, notably `pcnt` to `nullptr`. */
	SMesh(CDnmConvX& c) : p_(c), mlist(c), pcnt(nullptr) {};

    [[nodiscard]] std::string operator std::string() const {
        if (vts.empty()) {
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
        result += normal.operator std::string() + "\n}";
        return result;
    }
	void checkNormal(const std::string& mesh_name) {
    if (vts.empty() || fcs.empty()) {
        return;
    }
    if (normal.vts.size() != fcs.size()) {
        std::cerr << "Warning in SMesh::checkNormal for mesh '" << mesh_name
                  << "': Number of faces (" << fcs.size()
                  << ") does not match number of initial face normals provided (" << normal.vts.size()
                  << "). Cannot reliably calculate vertex normals." << std::endl;
        return;
    }

    SVertex zero_vertex;
    zero_vertex.x = 0.0f;
    zero_vertex.y = 0.0f;
    zero_vertex.z = 0.0f;
    zero_vertex.r = false;

    std::vector<SVertex> calculated_vertex_normals(vts.size(), zero_vertex);

    auto face_normal_it = normal.vts.cbegin();
    for (const auto& face : fcs) {
        const SVertex& current_face_normal = *face_normal_it;
        for (std::uint16_t vertex_index : face.vfi) {
            if (vertex_index < calculated_vertex_normals.size()) {
                calculated_vertex_normals[vertex_index] += current_face_normal;
            } else {
                std::cerr << "Warning in SMesh::checkNormal for mesh '" << mesh_name
                          << "': Vertex index " << vertex_index << " out of bounds for "
                          << calculated_vertex_normals.size() << " vertices." << std::endl;
            }
        }
        if (face_normal_it != normal.vts.cend()) {
             ++face_normal_it;
        }
    }

    std::uint16_t current_idx = 0;
    for (auto& vert_norm : calculated_vertex_normals) {
        vert_norm.normalize(mesh_name, current_idx++);
    }

    normal.vts.swap(calculated_vertex_normals);
}
	std::vector<std::uint16_t>listFaceIdx(const std::string&mt){
		std::vector<std::uint16_t>r_vec;
		std::uint16_t i_idx=0;
		for(const auto& val : mlist.mtIdx){
			if(val==mt)r_vec.push_back(i_idx);
			i_idx++;
		}
		return r_vec;
	}
	std::vector<std::uint16_t>listVertIdx(const std::vector<std::uint16_t>&fcid){
		std::map<std::uint16_t,std::uint16_t>vfMap;
		for(const auto& face_idx_val : fcid){
			for(const auto& vert_idx_val : fcs[face_idx_val].vfi){
				auto it_map = vfMap.find(vert_idx_val);
				if(it_map==vfMap.end())
					vfMap[vert_idx_val]=1;
				else ++vfMap[vert_idx_val];
			}
		}
		std::vector<std::uint16_t>r_vec;
		for(const auto& pair_ : vfMap){
			r_vec.push_back(pair_.first);
		}
		return r_vec;
	}
	std::vector<std::uint16_t>listUsedVertIdx(){
		std::vector<std::uint16_t>u_vec(vts.size(),0);
		for(const auto& face_val : fcs){
			for(const auto& v_idx : face_val.vfi){
				++u_vec[v_idx];
			}
		}
		return u_vec;
	}
	void invertFace(std::uint16_t idx){
		fcs[idx].reverseContent();
		normal.invertFace(idx);
	}
	void invertFace(std::string mt){
		checkNormal(name);
		std::map<std::uint16_t,SVertex>submh;
		SMesh mh_local(p_); // Ensure SMesh has a constructor that takes CDnmConvX&
		auto itf_it = fcs.begin();
		auto itn_it = normal.fcs.begin();
		auto itv_it = vts.begin();
		for(const auto& mat_name : mlist.mtIdx){
			if(mat_name==mt){
				itf_it->reverseContent();
				itn_it->reverseContent();
				mh_local.fcs.push_back(*itf_it);
				mh_local.vts.push_back(*itv_it);
				auto it_map = mlist.mtMap.find(mt);
				if(it_map!=mlist.mtMap.end())
					mh_local.mlist.mtMap[mt]=it_map->second;
			}
			++itf_it;++itn_it; ++itv_it;
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
		pcnt=nullptr;
		return*this;
	}
};

/** @brief Represents a quaternion for rotations. */
struct SQuaternion{
	float w; ///< W-component (scalar part).
	float x; ///< X-component (vector part).
	float y; ///< Y-component (vector part).
	float z; ///< Z-component (vector part).

	/** @brief Default constructor, initializes to identity quaternion (1,0,0,0). */
	SQuaternion() : w(1.0f), x(0.0f), y(0.0f), z(0.0f) {}
    /** @brief Constructor from individual components. */
    SQuaternion(float sw, float sx, float sy, float sz) : w(sw), x(sx), y(sy), z(sz) {}

	/** @brief Applies a rotation by `f` radians around the quaternion's current axis. (Assumes axis is already set, modifies w). */
	SQuaternion&rad(float f){
		const float hp(1.5707963f);
		float r_rad=hp*f,s_val; // Renamed r to r_rad, s to s_val
		s_val=std::sinf(r_rad);
		x*=s_val;y*=s_val;z*=s_val;
		w=std::cosf(r_rad);
		return*this;
	}
	SQuaternion&ri16(std::int32_t i){
		const float r_val=i/20860.756f;
		float s=std::sinf(r_val);
		x*=s;y*=s;z*=s;
		w=std::cosf(r_val);
		if(i>32768||i<-32768)w=-w;
		return*this;
	}
	[[nodiscard]] af9 getMatrix() const {
		af9 m;
		m[0]=1-2*y*y-2*z*z;m[1]=2*x*y-2*w*z;m[2]=2*x*z+2*w*y;
		m[3]=2*x*y+2*w*z;m[4]=1-2*x*x-2*z*z;m[5]=2*y*z-2*w*x;
		m[6]=2*x*z-2*w*y;m[7]=2*y*z+2*w*x;m[8]=1-2*x*x-2*y*y;
		return m;
	}
	[[nodiscard]] af3 rotateVert(SVertex&v) const {
		SVertex q_vert={x,y,z,false},r_vert;
		r_vert=v+(q_vert*2).cross(q_vert.cross(v)+(v*w));
		af3 a_val={r_vert.x,r_vert.y,r_vert.z};
		return a_val;
	}
	[[nodiscard]] af3 rotateVert(const af3&c_val) const {
		SVertex v={c_val[0],c_val[1],c_val[2],false};
		return rotateVert(v);
	}
	SQuaternion&operator=(const SVertex&v){
		w=0;
		x=v.x;y=v.y;z=v.z;
		return*this;
	}
	SQuaternion&operator=(const SQuaternion&q){
		w=q.w;x=q.x;y=q.y;z=q.z;
		return*this;
	}
	[[nodiscard]] SQuaternion operator*(const SQuaternion&q) const {
		SQuaternion r_val;
		r_val.w=(w*q.w-x*q.x-y*q.y-z*q.z);
		r_val.x=(w*q.x+x*q.w+y*q.z-z*q.y);
		r_val.y=(w*q.y-x*q.z+y*q.w+z*q.x);
		r_val.z=(w*q.z+x*q.y-y*q.x+z*q.w);
		return r_val;
	}
	SQuaternion&operator*=(const SQuaternion&q){
		const float r_arr[4]=
			{w*q.w-x*q.x-y*q.y-z*q.z
			,x*q.w+w*q.x-z*q.y+y*q.z
			,y*q.w+z*q.x+w*q.y-x*q.z
			,z*q.w-y*q.x+x*q.y+w*q.z};
		w=r_arr[0];x=r_arr[1];y=r_arr[2];z=r_arr[3];
		return*this;
	}
	SQuaternion&normalize(){
		float f=w*w+x*x+y*y+z*z,x2,y2,z2;
		std::int32_t i_val=static_cast<std::int32_t>(1000000*f);
		if(i_val!=1000000){
			f=std::sqrtf(f);
			x/=f;y/=f;z/=f;
			x2=x*x;y2=y*y;z2=z*z;
			w=std::sqrtf(1-x2-y2-z2);
		}
		return*this;
	}
	SQuaternion&ai16(const ai3&a_val){ // Renamed a to a_val
		SQuaternion t={0,0,1,0},p={0,1,0,0},b={0,0,0,1};
		t.ri16(a_val[0]);
		p.ri16(a_val[1]);
		b.ri16(a_val[2]);
		(*this)=(b*p)*t;
		return*this;
	}
    [[nodiscard]] operator std::string() const { // Already const, added nodiscard to match instructions
        return std::format("4;{:.6f},{:.6f},{:.6f},{:.6f};;", w, x, y, z);
    }
};

/** @brief Represents a 3x3 rotation matrix and a 3D translation vector. */
struct STransform{
	af9 a = {1.f,0.f,0.f, 0.f,1.f,0.f, 0.f,0.f,1.f}; ///< 3x3 rotation matrix components.
	af3 c = {0.f,0.f,0.f};                           ///< 3D translation vector components.

	/** @brief Default constructor, initializes to identity transform. */
	STransform() { reset(); }

	STransform&operator=(const STransform&t){
		a=t.a;
		c=t.c;
		return*this;
	}
	STransform&operator=(const af3&p_val){
		c=p_val;
		return*this;
	}
	STransform&operator=(const SVertex&p_val){
		c[0]=p_val.x;c[1]=p_val.y;c[2]=p_val.z;
		return*this;
	}
	STransform&operator=(const af9&m_val){
		a=m_val;
		return*this;
	}
	STransform&operator=(const ai3&i_val){
		const float p_const=10430.378350470453f;
		const bool keepCenter=true;
		STransform m_trans;
		if(i_val[2]){
			float z=i_val[2]/p_const;
			a[0]=std::cosf(z);a[1]=std::sinf(z);
			a[3]=-std::sinf(z);a[4]=std::cosf(z);
			if(i_val[1]){
				m_trans.reset(keepCenter);
				float x_val=i_val[1]/p_const; // Renamed x to x_val
				m_trans.a[4]=std::cosf(x_val);m_trans.a[5]=-std::sinf(x_val);
				m_trans.a[7]=std::sinf(x_val);m_trans.a[8]=std::cosf(x_val);
				(*this)*=m_trans;
			}
			if(i_val[0]){
				m_trans.reset(keepCenter);
				float y_val=i_val[0]/p_const; // Renamed y to y_val
				m_trans.a[0]=std::cosf(y_val);m_trans.a[2]=std::sinf(y_val);
				m_trans.a[6]=-std::sinf(y_val);m_trans.a[8]=std::cosf(y_val);
				(*this)*=m_trans;
			}
		}else if(i_val[1]){
				float x_val=i_val[1]/p_const; // Renamed x to x_val
				a[4]=std::cosf(x_val);a[5]=-std::sinf(x_val);
				a[7]=std::sinf(x_val);a[8]=std::cosf(x_val);
				if(i_val[0]){
					m_trans.reset(keepCenter);
					float y_val=i_val[0]/p_const; // Renamed y to y_val
					m_trans.a[0]=std::cosf(y_val);m_trans.a[2]=std::sinf(y_val);
					m_trans.a[6]=-std::sinf(y_val);m_trans.a[8]=std::cosf(y_val);
					(*this)*=m_trans;
				}
		}else if(i_val[0]){
			float y_val=i_val[0]/p_const; // Renamed y to y_val
			a[0]=std::cosf(y_val);a[2]=std::sinf(y_val);
			a[6]=-std::sinf(y_val);a[8]=std::cosf(y_val);
		}
		return*this;
	}
	STransform&operator=(SQuaternion q_val){ // Renamed q to q_val
		a=q_val.getMatrix();
		return*this;
	}
	[[nodiscard]] STransform operator*(STransform t) const {
		STransform r_val;
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
	[[nodiscard]] bool anyChange() const {
		return (c[0]||c[1]||c[2]||a[1]||a[2]||a[5]);
	}
	void reset(bool keepCenter=false){
		af9 t_arr={1,0,0,0,1,0,0,0,1};
		a.swap(t_arr);
		if(!keepCenter)
			c.fill(0.0f);
	}
    [[nodiscard]] operator std::string() const { // Already const, added nodiscard to match instructions
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
};
using ituQ = std::map<std::uint16_t,SQuaternion>::iterator;
using ituV = std::map<std::uint16_t,SVertex>::iterator;
using ituuu = std::map<std::uint16_t,std::map<std::uint16_t,std::uint16_t>>::iterator;
friend struct SAnimationKey;
struct SAnimationKey{
	SAnimationKey(CDnmConvX& c) : p_(c) {};
	CDnmConvX& p_;                                      ///< Reference to the main converter object.
	std::string name;                                   ///< Name of the frame/object being animated.
	std::map<std::uint16_t,SQuaternion> agMap;          ///< Map of keyframe times to rotation quaternions.
	std::map<std::uint16_t,SVertex> mvMap;              ///< Map of keyframe times to translation vertices.
	std::vector<std::string> parents;                   ///< Names of parent frames whose animations should be merged.
	SVertex c;                                          ///< Offset center for the animation, default initialized by SVertex().
	std::uint16_t cla;                                  ///< Type or class of animation (e.g., for different animation sequences).
	std::vector<af3> poss;                              ///< Pre-calculated world position coordinates for specific animation states.
	std::vector<ai3> tpbs;                              ///< Pre-calculated turn/pitch/bank angles for specific animation states.
	std::vector<bool> disps;                            ///< Visibility status at specific animation states.

	/** @brief Constructor. @param conv Reference to the main converter. Initializes `cla` to 0. */
	SAnimationKey(CDnmConvX& conv) : p_(conv), c(), cla(0) {};

	/** @brief Clears all data in the animation key, resetting it to a default state. */
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
		SVertex o_vert; // Renamed o to o_vert
		ai3&a_val=p_.frs.frMap[name].tpb; // Renamed a to a_val
		SQuaternion q_val=SQuaternion().ai16(a_val);
		if(&p_ != nullptr && !p_.otl.empty() && !tpbs.empty()){ // Check otl and tpbs for emptiness
			for(const auto& otl_pair : p_.otl){
				const std::uint16_t&k_val=otl_pair.first; // Renamed k to k_val
				std::uint16_t&i_val=otl_pair.second.at(cla);
				o_vert=poss[i_val];
				mvMap[k_val]=o_vert+c;
				ai3&b_val=tpbs[i_val];
				SQuaternion r_quat=SQuaternion().ai16(b_val);
				agMap[k_val]=r_quat*q_val;
			}
		}else{
			cla*=10;
			for(std::uint16_t i_idx=0;i_idx<tpbs.size();++i_idx){
				ai3&b_val=tpbs[i_idx];
				SQuaternion r_quat=SQuaternion().ai16(b_val);
				agMap[cla+i_idx]=r_quat*q_val;
				o_vert=poss[i_idx];
				mvMap[cla+i_idx]=o_vert+c;
			}
		}
	}
	void calcParent(const std::string&s=""){
		if(s.empty() || &p_ == nullptr)return; // Use .empty() for string check
		SAnimationKey&a_key=p_.frs.frMap[s].ak;
		a_key.calcSelf();
		for(const auto& otl_pair : p_.otl){
			const std::uint16_t&k_val=otl_pair.first; // Renamed k to k_val
			SQuaternion&r_quat=agMap[k_val];
			r_quat=r_quat*a_key.agMap[k_val];
		}
	}
    [[nodiscard]] operator std::string() const { // Already const, added nodiscard to match instructions
        std::string result = "AnimationKey{0;\n";
        result += std::format("{};\n", agMap.size());

        std::string agMap_str;
        for (const auto& pair_ : agMap) {
            agMap_str += std::format("{};{},\n", pair_.first, pair_.second.operator std::string());
        }
        if (!agMap_str.empty()) {
            agMap_str.pop_back();
            agMap_str.pop_back();
        }
        result += agMap_str;
        if (!agMap.empty()) result += "\n";
        result += ";\n";
        result += "}\nAnimationKey{2;\n";

        result += std::format("{};\n", mvMap.size());
        std::string mvMap_str;
        for (const auto& pair_ : mvMap) {
            mvMap_str += std::format("{};3;{:.6f},{:.6f},{:.6f};;,\n",
                                     pair_.first,
                                     pair_.second.x, pair_.second.y, pair_.second.z);
        }
        if (!mvMap_str.empty()) {
            mvMap_str.pop_back();
            mvMap_str.pop_back();
        }
        result += mvMap_str;
        if(!mvMap.empty()) result += "\n";
        result += ";\n";
        result += "}";
        return result;
    }
};
friend struct SFrame;

/**
 * @struct SFrame
 * @brief Represents a frame in the scene graph, including its transform, associated mesh, and animations.
 * Frames can be nested to create hierarchical structures.
 */
struct SFrame{
	CDnmConvX& p_;                      ///< Reference to the main converter object.
	bool nested;                        ///< Flag indicating if this frame is nested within another.
	std::string name;                   ///< Name of the frame.
	STransform ftm;                     ///< Transformation matrix of the frame.
	SVertex cnt;                        ///< Center point for a nested mesh, if any. Default initialized by SVertex().
	af3 pos;                            ///< Initial world position coordinates for this frame. Default initialized.
	ai3 tpb;                            ///< Initial turn/pitch/bank angles for this frame. Default initialized.
	bool disp;                          ///< Visibility status at its initial/still position.
	std::string mhId;                   ///< ID/Name of the mesh associated with this frame, if any.
	SAnimationKey ak;                   ///< Animation data for this frame.
	std::vector<std::string> frIds;     ///< IDs/Names of child frames nested under this frame.

	/**
	 * @brief Constructor.
	 * @param c Reference to the main converter object.
	 * Initializes frame to default state (e.g., not nested, false display, identity transform).
	 */
	SFrame(CDnmConvX& c) : p_(c), nested(false), ftm(), cnt(), pos{}, tpb{}, disp(false), ak(c) {
		// ftm is default constructed (which calls its reset())
		// cnt is default constructed
		// pos and tpb are value-initialized (zeroed) for std::array
		// ak is constructed with p_
	};

    void update_transform_and_animation_center() {
        for (std::uint8_t i = 0; i < 3; ++i) {
            ftm.c[i] += pos[i];
        }
        SQuaternion q_val = {1.0f, 0.0f, 0.0f, 0.0f};
        ftm = q_val.ai16(tpb);
		ak.c = ftm.c;
    }

	SFrame&operator=(const SFrame&f){
		// p_ reference is initialized at construction and should not be reassigned.
		nested = f.nested;
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
		cnt=SVertex{0.0f, 0.0f, 0.0f, false}; // Explicitly initialize SVertex members
		pos.fill(0.0f);
		tpb.fill(0);
		disp=false;
		mhId.clear();
		ak.clear();
		frIds.clear();
	}

    [[nodiscard]] operator std::string() const { // Added nodiscard as per instruction
        if (name.empty()) return "{}";

        std::string result;
        result += std::format("Frame {} {{\n", name);

        if (ftm.anyChange()) {
            result += ftm.operator std::string() + "\n";
        }

        if (!mhId.empty() && mhId != "null") {
             result += std::format("{{{{{}}}}}\n", mhId);
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
};
using itsMT_ = std::map<std::string,SMaterial>::const_iterator;
struct SMapCollMat{
	std::map<std::string,SMaterial> mtMap;       ///< Map storing materials, keyed by material name.

	/** @brief Adds or updates a material in the collection.
	 *  @param m_val The material to add/update. */
	SMapCollMat&operator<<(const SMaterial&m_val){
		if(!m_val.name.empty())mtMap[m_val.name]=m_val;
		return*this;
	}
	/** @brief Extracts the first material from the collection (destructive read).
	 *  @param m_val Output parameter to receive the extracted material. */
	SMapCollMat&operator>>(SMaterial&m_val){
		itsMT it=mtMap.begin();
		if (it != mtMap.end()) { // Check if map is not empty
			m_val=it->second;
			mtMap.erase(it);
		}
		return*this;
	}
	/** @brief Merges materials from another SMaterialList into this collection.
	 *  @param ml_val The SMaterialList whose materials are to be merged. */
	SMapCollMat&operator<<(const SMaterialList&ml_val){
		for(const auto& pair_ : ml_val.mtMap)
			mtMap[pair_.first]=pair_.second;
		return*this;
	}
	/** @brief Extracts all materials into a SMaterialList (destructive read from this collection).
	 *  @param ml_val Output parameter to receive the list of materials. */
	SMapCollMat&operator>>(SMaterialList&ml_val){
		ml_val.mtMap.swap(mtMap); // Efficiently move materials
		mtMap.clear();            // Clear this collection
		return*this;
	}
	/** @brief Generates a string representation of all materials in X file format. */
	[[nodiscard]] std::string operator std::string() const {
		std::string result;
		for (const auto& pair_ : mtMap) {
			result += pair_.second.operator std::string() + "\n";
		}
		result += "\n";
		return result;
	}
};
using itsMH = std::map<std::string,SMesh>::iterator;
friend struct SMapCollMsh;
struct SMapCollMsh{
	SMapCollMsh(CDnmConvX& c) : p_(c) {};
	CDnmConvX& p_;                             ///< Reference to the main converter object.
	std::map<std::string,SMesh> mhMap;         ///< Map storing meshes, keyed by mesh name.

	/** @brief Constructor. @param c Reference to the main converter. */
	SMapCollMsh(CDnmConvX& c) : p_(c) {};

	/** @brief Adds or updates a mesh in the collection.
	 *  @param m_val The mesh to add/update. */
	SMapCollMsh&operator<<(const SMesh&m_val){
		if(!m_val.name.empty())mhMap[m_val.name]=m_val;
		return*this;
	}
	/** @brief Extracts the first mesh from the collection (destructive read).
	 *  @param m_val Output parameter to receive the extracted mesh. */
	SMapCollMsh&operator>>(SMesh&m_val){
		itsMH it=mhMap.begin();
		if (it != mhMap.end()) { // Check if map is not empty
			m_val=it->second;
			mhMap.erase(it); // Remove the extracted mesh
		}
		return*this;
	}
	/** @brief Generates a string representation of all meshes in X file format. */
    [[nodiscard]] operator std::string() const {
        std::string result;
        for (const auto& pair_ : mhMap) {
            if (!pair_.first.empty()) { // Ensure mesh name is not empty
                result += pair_.second.operator std::string() + "\n";
            }
        }
        result += "\n";
        return result;
    }
};
using itsFR = std::map<std::string,SFrame>::iterator;
friend struct SMapCollFrm;
struct SMapCollFrm{
	SMapCollFrm(CDnmConvX& c) : p_(c) {};
	CDnmConvX& p_;                             ///< Reference to the main converter object.
	std::map<std::string,SFrame> frMap;        ///< Map storing frames, keyed by frame name.

	/** @brief Constructor. @param c Reference to the main converter. */
	SMapCollFrm(CDnmConvX& c) : p_(c) {};

	/** @brief Adds or updates a frame in the collection.
	 *  @param f_val The frame to add/update.
	 *  @warning If a frame with `f_val.name` does not exist, `frMap[f_val.name]` creates a new `SFrame`
	 *           which requires `CDnmConvX&`. This operation is safe if `SFrame` is already in map or
	 *           if map default construction + assignment is well-defined for `SFrame` (which it is not due to ref member).
	 *           This implies that this operator should primarily be used to update existing frames or after ensuring
	 *           a frame with `f_val.name` and correct `p_` has been emplaced. */
	SMapCollFrm&operator<<(const SFrame&f_val){
		if(!f_val.name.empty()){
			auto it = frMap.find(f_val.name);
			if (it != frMap.end()) {
				it->second = f_val; // Assign if exists
			} else {
				// Potentially problematic if SFrame cannot be default constructed or if p_ is not set correctly.
				// For safety, one might prefer emplace: frMap.emplace(f_val.name, f_val);
				// However, SFrame f_val already has its p_ from its own construction.
				// If f_val is a temporary or a copy not tied to this specific CDnmConvX's p_ context, that's an issue.
				// Assuming f_val is correctly constructed with the same p_ context or copy is deep enough.
				frMap[f_val.name] = f_val;
			}
		}
		return*this;
	}
	/** @brief Extracts the first frame from the collection (destructive read).
	 *  @param f_val Output parameter to receive the extracted frame. */
	SMapCollFrm&operator>>(SFrame&f_val){
		itsFR it=frMap.begin();
		if (it != frMap.end()) { // Check if map is not empty
			f_val=it->second;
			frMap.erase(it);
		}
		return*this;
	}
	/** @brief Generates a string representation of all top-level frames in X file format. */
    [[nodiscard]] operator std::string() const {
        std::string result;
        for (const auto& pair_ : frMap) {
            if (!pair_.second.nested && !pair_.second.name.empty()) { // Output only non-nested (top-level) frames
                result += pair_.second.operator std::string() + "\n";
            }
        }
        result += "\n";
        return result;
    }
};
using itsA = std::map<std::string,SAnimationKey>::iterator;
friend struct SMapCollAnim;
struct SMapCollAnim{
	SMapCollAnim(CDnmConvX& c) : p_(c) {};
	CDnmConvX& p_;                                     ///< Reference to the main converter object.
	std::map<std::string,SAnimationKey> akMap;         ///< Map storing animation keys, keyed by an associated name (often a frame name).

	/** @brief Constructor. @param c Reference to the main converter. */
	SMapCollAnim(CDnmConvX& c) : p_(c) {};

	/** @brief Adds or updates an animation key in the collection.
	 *  @param ak_val The animation key to add/update.
	 *  @warning Similar caveats as SMapCollFrm::operator<< regarding map access and object construction. */
	SMapCollAnim&operator<<(SAnimationKey&ak_val){
		// Assuming SAnimationKey is correctly constructed with the same p_ context or copy is deep enough.
		akMap[ak_val.name]=ak_val;
		return*this;
	}
	/** @brief Generates a string representation of all animation sets in X file format. */
    [[nodiscard]] operator std::string() const {
        std::string result = "AnimationSet{\n";
        for (const auto& pair_ : akMap) {
            if (!pair_.second.name.empty()) {
                result += std::format("Animation{{\n{{}}\n"
                                    "{}\n"
                                    "AnimationOptions{{0;0;}}\n}}\n",
                                    pair_.first,
                                    pair_.second.operator std::string());
            }
        }
        result += "}\n";
        return result;
    }
};

private:
	std::string inFilePath;                             ///< Path to the input DNM file.
	std::string inFilePath;                             ///< Path to the input DNM file being processed.
	SMapCollMat mts, omts;                              ///< `mts`: Collection of materials parsed directly from the DNM. `omts`: Collection of materials prepared for output (potentially modified by INI).
	SMapCollMsh mhs, omhs;                              ///< `mhs`: Collection of meshes parsed directly from the DNM. `omhs`: Collection of meshes prepared for output.
	SMapCollFrm frs;                                    ///< Collection of frames (scene hierarchy nodes) parsed from the DNM.
	SMapCollAnim aks;                                   ///< Collection of animation key sets parsed from the DNM.
	bool nstmt = false;                                 ///< Config flag: If true, output materials nested within MeshMaterialList in the X file. Otherwise, output globally and reference by name.
	bool nstmh = false;                                 ///< Config flag: If true, output meshes nested within their respective Frame definitions in the X file. Otherwise, define meshes globally.
	bool mnm = false;                                   ///< Config flag: If true, special handling for "null" meshes - merge their frame/animation data. (YSFlight specific convention).
	std::vector<std::string> configs;                   ///< Stores general configuration lines read from the INI file.
	std::vector<std::string> mhbl;                      ///< Mesh blacklist: list of mesh names (strings) to be excluded from the output X file.
	std::vector<std::string> frbl;                      ///< Frame blacklist: list of frame names (strings) to be excluded from the output X file.
	std::vector<std::string> dsmh;                      ///< Double-sided meshes: list of mesh names that should be rendered as double-sided in the X file.
	std::vector<std::string> invfidx;                   ///< Invert Face by Index: list of strings, each specifying "meshName,faceIndex" for faces whose winding order should be inverted.
	std::vector<std::string> invfmt;                    ///< Invert Face by Material: list of strings, each specifying "meshName,materialName" for which all faces using that material in that mesh should be inverted.
	std::map<std::uint16_t,std::map<std::uint16_t,std::uint16_t>> otl; ///< Output Timeline Keyframes: A complex map structure read from INI, likely defining how raw animation states (`cla`, `sta` from DNM) map to specific output keyframe times in the X file. `map<dnm_cla, map<dnm_state_idx, x_keyframe_time>>`.
};

#endif // DNMCONVX_H_
