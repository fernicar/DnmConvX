#pragma once
#ifndef _DNMCONVX_
#define _DNMCONVX_
#include<array>		// std::tr1::array<>
#include<string>	// string
#include<sstream>	// istringstream stringstream
#include<iostream>	// cout endl getchar() ostream
#include<iomanip>	// setprecision() hex setw() setfill() right
#include<fstream>	// ifstream ofstream
#include<vector>	// vector<>
#include<map>		// map<> make_pair()
#include<math.h>	// sqrtf() cos() sin()
#include<functional>
#include<regex>
using std::tr1::array;

using namespace std;

typedef const char*		cstr;
typedef char			c08;
typedef unsigned char	u08;

typedef long int		i32;
typedef short int		i16;
typedef unsigned long	u32;
typedef unsigned short	u16;

typedef float			f32;
typedef array<i32,3>	ai3;
typedef array<f32,3>	af3;
typedef array<f32,9>	af9;
typedef vector<string>	vs;
typedef vector<i32>		vi;
typedef vector<u16>		vu;
typedef vector<f32>		vf;
typedef map<u16,u16>	uu;

typedef vs::const_iterator	it_vs_;
typedef vs::iterator		it_vs;
typedef vi::const_iterator	it_vi_;
typedef vi::iterator		it_vi;
typedef vu::const_iterator	it_vu_;
typedef vu::iterator		it_vu;
typedef vf::const_iterator	it_vf_;
typedef vf::iterator		it_vf;
typedef uu::const_iterator	it_uu_;
typedef uu::iterator		it_uu;

#pragma warning(push)	// start of disableSpecificWarnings
#pragma warning(disable:4258)

// equivalent trick for(auto&it:container) from c++14
#define each(i,c) for(i i=(c).begin();(c).end()!=i;++i)

#define RETURN_CONST_C_STR(ss)\
	{static string st;\
	st=ss.str();\
	return st.c_str();}

#define FLOAT_PRECISION(q)\
	fixed<<setprecision(q);

enum E_ERROR{
	E_NotError,
	E_IniPath,
	E_IniRead,
	E_MemoryError,
	E_DnmRead,
	E_XWrite,
	E_Error
};

typedef enum E_flib{NOT=false,YES=true}flib;
inline flib&operator++(flib&b)		{return b=YES;}				// prefix ++
inline flib operator++(flib&b,int)	{flib t=b;++b;return t;}	// postfix ++
inline flib&operator--(flib&b)		{return b=NOT;}				// prefix --
inline flib operator--(flib&b,int)	{flib t=b;--b;return t;}	// postfix --
//inline flib&operator=(flib&f,const bool&b){return f=b?YES:NOT}

class CDnmConvX{
public:
	CDnmConvX(void);
// 	~CDnmConvX(void);
	CDnmConvX&operator<<(istringstream&iss);
	operator cstr();
	u16 outputToXFile(cstr outPath="");
	u16 inputDnmFile(cstr inPath);
	u16 inputIniFile(cstr inPath);

private:
friend struct SMaterialList;
friend struct SMapCollMsh;
friend struct SMapCollFrm;
friend struct SMapCollAnim;

typedef union UColor15Bit{
	u16 u;								// init helper (max 15bit valid 0x7FFF=32767)
	struct{
		u16 r:5;						// red
		u16 g:5;						// green
		u16 b:5;						// blue
		u16 p:1;						// padding garbage, total=16bit
	};
}c15b;
typedef union UColor24Bit{
	struct{
		u16 r:8;						// red
		u16 g:8;						// green
		u16 b:8;						// blue
		u16 a:8;						// alpha
	};
	u32 u;								// init helper, size 32bit
	UColor24Bit&operator=(const UColor15Bit&c){
		r=c.r*8+c.r/4;
		g=c.g*8+c.g/4;
		b=c.b*8+c.b/4;
		return*this;}
	operator cstr(){					// c_str compatibility
		stringstream ss;
		ss	<<hex<<setfill('0')<<right
			<<'_'<<setw(2)<<r<<setw(2)<<g<<setw(2)<<b<<'_';
		RETURN_CONST_C_STR(ss);
	}
}c24b;
typedef struct SVertex{
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
	SVertex operator-(const SVertex&v){
		SVertex a={x-v.x,y-v.y,z-v.z,false};
		return a;
	}
	SVertex operator+(const SVertex&v){
		SVertex a={x+v.x,y+v.y,z+v.z,false};
		return a;
	}
	operator cstr(){					// c_str compatibility
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		ss<<x<<';'<<y<<';'<<z<<';';
		static string st[3];			// 3 line buffer
		static u16 i(0);				// track the last one filled
		st[++i%=3]=ss.str();			// fill the next one
		return st[i].c_str();			// emulate const chart pointer return
	}
	f32 dot(const SVertex&v){
		return (x*v.x+y*v.y+z*v.z);
	}
	SVertex cross(const SVertex&v){
		SVertex r=	{y*v.z-z*v.y
					,z*v.x-x*v.z
					,x*v.y-y*v.x};
		return r;
	}
	f32 length(){return sqrtf(x*x+y*y+z*z);}
	void normalize(const string&name="",const u16 idx=0){
		f32 len=length(),mod=1;
		if(x==0&&y==0&&z==0)
			cerr<<name<<" V:"<<setw(4)<<right<<idx<<" invalid vertex normal\n";
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
	SVertex operator*(const f32&f){
		SVertex n={x*f,y*f,z*f,false};
		return n;
	}
	bool operator!=(const SVertex&v){return (x!=v.x||y!=v.y||z!=v.z);}
	bool operator!=(const af3&v){return (x!=v[0]||y!=v[1]||z!=v[2]);}
}vertex;
typedef vector<vertex>::iterator itvV;
typedef struct SFaceIdx{
	vector<u16>vfi;
	void reverseContent(){
		vector<u16>r(vfi.rbegin(),vfi.rend());
		vfi.swap(r);
	}
	operator cstr(){					// c_str compatibility
		if(vfi.size()==0)return "";
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		ss<<vfi.size()<<';';
		each(it_vu_,vfi)
			ss<<*it_vu_<<',';
		ss.seekp(-1,ss.end)<<';';		// overwrite last char from ',' to ';'
		RETURN_CONST_C_STR(ss);
	}
}faceIdx;
typedef vector<faceIdx>::iterator itvFI;
typedef struct SColor3Float{
	f32 r,g,b;
	SColor3Float&operator=(const c24b&c24){
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
	operator cstr(){
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		ss<<r<<';'<<g<<';'<<b;
		static string st[3];		// 3 line buffer
		static u16 i(0);			// track the last one filled
		st[++i%=3]=ss.str();		// fill the next one
		return st[i].c_str();		// emulate const chart pointer return
	}
}cl3f;
typedef struct SMaterial{
	u16 i;							// material list index
	string name;
	cl3f d;							// diffuse RGB
	f32 a;							// alpha
	f32 g;							// gloss
	cl3f s;							// specular RGB
	cl3f e;							// emissive RGB
	operator cstr(){
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		ss	<<"Material "<<name<<"{\n"
			<<d<<";"<<a<<";;\n"
			<<g<<";\n"
			<<s<<";;\n"
			<<e<<";;\n}";
		RETURN_CONST_C_STR(ss);
	}
}material;
typedef map<string,material>::iterator itsMT;
typedef struct SMaterialList{
	SMaterialList(CDnmConvX&c):p(&c){};
	CDnmConvX*p;
	vector<string>mtIdx;						// material index per vertex
	map<string,material>mtMap;
	SMaterialList():p(NULL){}
	operator cstr(){
		u32 sz=mtIdx.size();
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		ss	<<"MeshMaterialList {\n"
			<<mtMap.size()<<";\n"
			<<mtIdx.size()<<";\n";
		u16 i=0;
		each(itsMT,mtMap)
			itsMT->second.i=i++;				// re-indexing all mat idx
		each(it_vs,mtIdx)
			ss<<mtMap[*it_vs].i<<',';
//			ss<<mtMap[*it_vs].i<<",\n";
		ss.seekp(-1,ss.end)<<";;\n";			// overwrite last char from ',' to ";;\n"
//		ss.seekp(-2,ss.end)<<";;\n";			// overwrite last 2 char from ",\n" to ";;\n"
		if(p)
			each(itsMT,mtMap)					// loop each material name
				if(p->nstmt)					// true when nesting material
					ss<<itsMT->second<<endl;	// output whole material
				else{							// true when output material definitions
					p->omts.mtMap.insert(*itsMT);	// store output material
					ss<<"{"<<itsMT->second.name<<"}\n";	// output material name only
				}
		ss<<"}";
		RETURN_CONST_C_STR(ss);
	}
}mlist;
typedef struct SMeshNormals{
	vector<vertex>vts;						// faces normal coords to vertex normal coords
	vector<faceIdx>fcs;						// redundant faces
	void invertNormal(u16 idx){
		vts[idx].invert();
	}
	void invertFace(u16 idx){
		fcs[idx].reverseContent();
	}
	operator cstr(){
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		ss<<"MeshNormals{\n"<<vts.size()<<';';
		each(itvV,vts)
			ss<<endl<<*itvV<<',';
		ss.seekp(-1,ss.end)<<";\n"<<fcs.size()<<';';	// overwrite last char from ',' to ";\n"
		each(itvFI,fcs)
			ss<<endl<<*itvFI<<',';
		ss.seekp(-1,ss.end)<<";\n}";			// overwrite last char from ',' to ";\n"
		RETURN_CONST_C_STR(ss);
	}
}normal;
friend struct SMesh;
typedef struct SMesh{
	SMesh(CDnmConvX&c):p(&c),mlist(c){};
	CDnmConvX*p;
	string name;
	vector<vertex>vts;							// indexed vertex
	vector<faceIdx>fcs;							// Indexed faces
	normal normal;
	mlist mlist;
	vertex*pcnt;								// reference to new mesh center
	SMesh():p(NULL),pcnt(NULL),mlist(*p){}
	operator cstr(){
		if(vts.size()==0){						// true if empty mesh
			stringstream ss;
			ss<<"Mesh "<<name<<"{1;0;0;0;;1;3;0,0,0;;}";	// default output
			RETURN_CONST_C_STR(ss);
		}
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		ss<<"Mesh "<<name<<"{\n"<<vts.size()<<';';
		vector<vertex>v;
		if(pcnt&&pcnt->anyChange())				// true when new mesh center
			each(itvV,vts)
				v.push_back((*itvV)-*pcnt);		// apply new center
		else v=vts;
		each(itvV,v)							// each vertex
			ss<<endl<<*itvV<<',';
		ss.seekp(-1,ss.end)<<";\n"<<fcs.size()<<';';
		each(itvFI,fcs)							// each face
			ss<<endl<<*itvFI<<',';
		ss.seekp(-1,ss.end)<<";\n";
		ss<<mlist<<"\n";						// output MaterialList
		checkNormal(name);						// update all normals
		ss<<normal<<"\n}";						// output updated normals
		RETURN_CONST_C_STR(ss);
	}
	void checkNormal(const string&name){
		if(vts.size()==normal.vts.size())return;
		vertex v={0,0,0,false};					// initializer vertex
		vector<vertex>n(vts.size(),v);			// random access of normal vertex
		itvV it=normal.vts.begin();				// group of normal vertex to sum
		each(itvFI,fcs){						// iterate each 144 faces
			each(it_vu_,itvFI->vfi)				// and each 4 or 3 vertex in face
				n[*it_vu_]+=(*it);				// sum normal vertex to n[vertIdx]
			++it;								// next normal vertex
		}
		u16 idx=-1;
		each(itvV,n)							// after sum all normals
			itvV->normalize(name,++idx);		// normalize each
		normal.vts.swap(n);						// save the normalized group
		return;
	}
	vector<u16>listFaceIdx(const string&mt){
		vector<u16>r;
		u16 i=0;								// count faces index
		each(it_vs,mlist.mtIdx){				// mtIdx.size=fcs.size
			if(*it_vs==mt)r.push_back(i);		// true on range 22~44 46~49
			i++;
		}										// r.size 25 faces
		return r;
	}
	vector<u16>listVertIdx(const vector<u16>&fcid){
		map<u16,u16>vfMap;						// store vertex idx and its face count
		u16 i=0;								// count vertex index
		each(it_vu_,fcid){						// each face index
			each(it_vu,fcs[*it_vu_].vfi){		// each vertex index in face
				it_uu it=vfMap.find(*it_vu);	// find vertex before saving
				if(it==vfMap.end())				// true if new vertex index
					vfMap[*it_vu]=1;			// initialize
				else ++vfMap[*it_vu];			// else increase face count
			}
		}
		vector<u16>r;
		each(it_uu,vfMap){
			r.push_back(it_uu->first);
		}
		return r;
	}
	vector<u16>listUsedVertIdx(){
		vector<u16>u(vts.size(),0);				// store how many face use each vertex
		each(itvFI,fcs){						// each face index
			each(it_vu_,itvFI->vfi){			// each vertex index in face
				++u[*it_vu_];					// increase face count
			}
		}
		return u;
	}
	void invertFace(u16 idx){
		fcs[idx].reverseContent();
		normal.invertFace(idx);
	}
	void invertFace(string mt){
		checkNormal(name);
		map<u16,vertex>submh;
		mesh mh;
		itvFI itf=fcs.begin(),itn=normal.fcs.begin();
		itvV  itv=vts.begin();
		each(it_vs_,mlist.mtIdx){			// each string in material index, range 22~44 46~49
			if(*it_vs_==mt){						// true on range 22~44 46~49
				itf->reverseContent();				// reverse face vertex index
				itn->reverseContent();				// reverse normal vertex index
				// need map<vertIdx,vertCoord> of each vert in itf
				mh.fcs.push_back(*itf);				// face index
				mh.vts.push_back(*itv);				// vertex coords
				// normal face index, vertex coords
				itsMT it=mlist.mtMap.find(mt);		// get material
				if(it!=mlist.mtMap.end())
					mh.mlist.mtMap[mt]=it->second;	// copy material
			}
			++itf;++itn;							// next face, next normal
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
}mesh;
typedef struct SQuaternion{
	f32 w,x,y,z;
	SQuaternion&rad(f32 f){
		const f32 hp(1.5707963f);				// pi/2
		f32 r=hp*f,s;
		s=sinf(r);
		x*=s;y*=s;z*=s;
		w=cosf(r);
//		w=sqrtf(1-x*x-y*y-z*z);					// alternative calc
		return*this;
	}
	SQuaternion&ri16(i32 i){					// valid -32786 up to 32768
//		const f32 p=20860.756f;					// 65536/pi to 32bits equivalent
		const f32 r=i/20860.756f;
		f32 s=sinf(r);
		x*=s;y*=s;z*=s;
		w=cosf(r);
//		w=sqrtf(1-x*x-y*y-z*z);					// alternative calc
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
	af3 rotateVert(vertex&v){
		vertex q={x,y,z,false},r;
		r=v+(q*2).cross(q.cross(v)+(v*w));
		af3 a={r.x,r.y,r.z};
		return a;
	}
	af3 rotateVert(const af3&c){
		vertex v={c[0],c[1],c[2],false};
		return rotateVert(v);
	}
	SQuaternion&operator=(const vertex&v){
		w=0;
		x=v.x;y=v.y;z=v.z;
		return*this;
	}
	SQuaternion&operator=(const SQuaternion&q){
		w=q.w;x=q.x;y=q.y;z=q.z;
		return*this;
	}
	SQuaternion operator*(const SQuaternion&q){
		SQuaternion r;
		r.w=(w*q.w-x*q.x-y*q.y-z*q.z);
		r.x=(w*q.x+x*q.w+y*q.z-z*q.y);
		r.y=(w*q.y-x*q.z+y*q.w+z*q.x);
		r.z=(w*q.z+x*q.y-y*q.x+z*q.w);
		return r;
	}
	SQuaternion&operator*=(const SQuaternion&q){
		const f32 r[4]=
			{w*q.w-x*q.x-y*q.y-z*q.z
			,x*q.w+w*q.x-z*q.y+y*q.z
			,y*q.w+z*q.x+w*q.y-x*q.z
			,z*q.w-y*q.x+x*q.y+w*q.z};
		w=r[0];x=r[1];y=r[2];z=r[3];
		return*this;
	}
	SQuaternion&normalize(){
		f32 f=w*w+x*x+y*y+z*z,x2,y2,z2;
		i32 i=static_cast<i32>(1000000*f);
		if(i!=1000000){
			f=sqrtf(f);
			x/=f;y/=f;z/=f;
			x2=x*x;y2=y*y;z2=z*z;
			w=sqrtf(1-x2-y2-z2);
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
	operator cstr(){
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		ss<<"4;"<<w<<','<<x<<','<<y<<','<<z<<";;";
		RETURN_CONST_C_STR(ss);
	}
}quat;
typedef struct STransform{
	af9 a;										// angle of rotation
	af3 c;										// center of translation
	STransform&operator=(const STransform&t){
		a=t.a;
		c=t.c;
		return*this;
	}
	STransform&operator=(const af3&p){
		c=p;
		return*this;
	}
	STransform&operator=(const vertex&p){
		c[0]=p.x;c[1]=p.y;c[2]=p.z;
		return*this;
	}
	STransform&operator=(const af9&m){
		a=m;
		return*this;
	}
	STransform&operator=(const ai3&i){
		const f32 p=10430.378350470453f;		// 32768/pi
		const bool keepCenter=true;
		STransform m;
		if(i[2]){								// true if bank angle
			f32 z=i[2]/p;						// bank left/right (left handed)
			a[0]=cosf(z);a[1]=sinf(z);
			a[3]=-sinf(z);a[4]=cosf(z);
			if(i[1]){							// true if bank and pith angle
				m.reset(keepCenter);
				f32 x=i[1]/p;					// pith up/down (right handed)
				m.a[4]=cosf(x);m.a[5]=-sinf(x);
				m.a[7]=sinf(x);m.a[8]=cosf(x);
				(*this)*=m;						// aggregated
			}
			if(i[0]){							// true if bank, pitch and turn angle
				m.reset(keepCenter);
				f32 y=i[0]/p;					// turn around west/east (right handed)
				m.a[0]=cosf(y);m.a[2]=sinf(y);
				m.a[6]=-sinf(y);m.a[8]=cosf(y);
				(*this)*=m;						// aggregated
			}
		}else if(i[1]){							// true if pitch angle
				f32 x=i[1]/p;					// bank left/right (right handed)
				a[4]=cosf(x);a[5]=-sinf(x);
				a[7]=sinf(x);a[8]=cosf(x);
				if(i[0]){						//true if pitch and turn angle
					m.reset(keepCenter);
					f32 y=i[0]/p;				// turn around west/east (right handed)
					m.a[0]=cosf(y);m.a[2]=sinf(y);
					m.a[6]=-sinf(y);m.a[8]=cosf(y);
					(*this)*=m;					// aggregated
				}
		}else if(i[0]){							// true if turn angle only
			f32 y=i[0]/p;						// turn around west/east (right handed)
			a[0]=cosf(y);a[2]=sinf(y);
			a[6]=-sinf(y);a[8]=cosf(y);
		}
		return*this;
	}
	STransform&operator=(quat q){
		a=q.getMatrix();
//		c=q.rotateVert(c);
		return*this;
	}
	STransform operator*(STransform t){
		STransform r;
		r.a[0]=a[0]*t.a[0]+a[1]*t.a[3]+a[2]*t.a[6];
		r.a[1]=a[0]*t.a[1]+a[1]*t.a[4]+a[2]*t.a[7];
		r.a[2]=a[0]*t.a[2]+a[1]*t.a[5]+a[2]*t.a[8];
		r.a[3]=a[3]*t.a[0]+a[4]*t.a[3]+a[5]*t.a[6];
		r.a[4]=a[3]*t.a[1]+a[4]*t.a[4]+a[5]*t.a[7];
		r.a[5]=a[3]*t.a[2]+a[4]*t.a[5]+a[5]*t.a[8];
		r.a[6]=a[6]*t.a[0]+a[7]*t.a[3]+a[8]*t.a[6];
		r.a[7]=a[6]*t.a[1]+a[7]*t.a[4]+a[8]*t.a[7];
		r.a[8]=a[6]*t.a[2]+a[7]*t.a[5]+a[8]*t.a[8];
		r.c=this->c;
		return r;
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
	operator cstr(){
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		ss<<"FrameTransformMatrix{\n"
		  <<a[0]<<','<<a[1]<<','<<a[2]<<",0,\n"
		  <<a[3]<<','<<a[4]<<','<<a[5]<<",0,\n"
		  <<a[6]<<','<<a[7]<<','<<a[8]<<",0,\n"
		  <<c[0]<<','<<c[1]<<','<<c[2]<<",1.0;;\n}";
		RETURN_CONST_C_STR(ss);
	}
}mtx;
typedef map<u16,quat>::iterator ituQ;
typedef map<u16,vertex>::iterator ituV;
typedef map<u16,map<u16,u16>>::iterator ituuu;
friend struct SAnimationKey;
typedef struct SAnimationKey{					// Animation{
	SAnimationKey(CDnmConvX&c):p(&c){};
	CDnmConvX*p;
	string name;								// name of frame to animate
	map<u16,quat>agMap;							// map of keys and angles
	map<u16,vertex>mvMap;						// map of keys and movement
	vector<string>parents;						// list of frames to merge with
	vertex c;									// new offset center
	u16 cla;									// type of anim
	vector<af3>poss;							// world position coordinates
	vector<ai3>tpbs;							// 3 angles -32768 up to 32768(65536+1)
	vector<bool>disps;							// visible status at animation state coord
	SAnimationKey():p(NULL){};
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
		vertex o;
		ai3&a=p->frs.frMap[name].tpb;			// shortcut to current frame angle
		quat q=q.ai16(a);						// rotate using current frame angle
		if(p&&p->otl.size()&&tpbs.size()){		// true when there is pose config
			each(ituuu,p->otl){					// each required keyframe from config
				const u16&k=ituuu->first;		// shortcut to current keyframe from config
				u16&i=ituuu->second[cla];		// shortcut to cla status from config
				o=poss[i];						// convert new position coords to vertex
				mvMap[k]=o+c;					// output position animkey and frame origin
				ai3&b=tpbs[i];					// shortcut to current animkey angles
				quat r=r.ai16(b);				// turn, pitch and bank animkey
				agMap[k]=r*q;					// output animkey and frame orientation
			}
		}else{									// output all posible animkey sorted by cla
			cla*=10;
			for(u16 i=0;i<tpbs.size();++i){
				ai3&b=tpbs[i];					// shortcut to current animkey angles
				quat r=r.ai16(b);				// turn, pitch and bank animkey
				agMap[cla+i]=r*q;				// output animkey and frame orientation
				o=poss[i];						// get new position coords
				mvMap[cla+i]=o+c;				// update animkey position
			}
		}
	}
	void calcParent(const string&s=""){
		if(s==""||!p)return;
		SAnimationKey&a=p->frs.frMap[s].ak;
		a.calcSelf();
		each(ituuu,p->otl){
			const u16&k=ituuu->first;
			quat&r=agMap[k];
			r=r*a.agMap[k];
//			vertex&o=mvMap[k];
//			o=o+mvMap[k];
		}
	}
	operator cstr(){
		calcSelf();
		each(it_vs_,parents)
			calcParent(*it_vs_);
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		ss<<"AnimationKey{0;\n"
			<<agMap.size()<<";\n";
		each(ituQ,agMap)
			ss<<ituQ->first<<';'<<ituQ->second<<",\n";
		ss.seekp(-2,ss.end);ss<<";\n";		// overwrite last 2 chars from ",\n" to ";\n"
		ss<<"}\nAnimationKey{2;\n"
			<<mvMap.size()<<";\n";
		each(ituV,mvMap)
			ss<<ituV->first<<";3;"
			<<ituV->second.x<<','
			<<ituV->second.y<<','
			<<ituV->second.z<<";;,\n";
		ss.seekp(-2,ss.end);ss<<";\n";		// overwrite last 2 chars from ",\n" to ";\n"
		ss<<"}";
		RETURN_CONST_C_STR(ss);
	}
}anikey;
friend struct SFrame;
typedef struct SFrame{
	SFrame(CDnmConvX&c):p(&c),nested(false),ak(*p){ftm.reset();};
	CDnmConvX*p;
	bool nested;
	string name;
	mtx ftm;									// FrameTransformMatrix
	vertex cnt;									// new center for the nested mesh
	af3 pos;									// world position coordinates
	ai3 tpb;									// 3 angles -32768 up to 32768(65536+1)
	bool disp;									// visible status at still position coord
	string mhId;								// nested mesh
	anikey ak;									// animation of the frame
	vector<string>frIds;						// id of nested frames
	SFrame():p(NULL),nested(false),ak(*p){ftm.reset();}
	SFrame&operator=(const SFrame&f){			// nested should not copy
		p=f.p;
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
		cnt=vertex{0};
		pos.empty();
		tpb.empty();
		disp=false;
		mhId.clear();
		ak.clear();
		frIds.clear();
	}
	operator cstr(){
		if(name=="")return"{}";					// true if blacklisted frame
		// animation here
		for(u08 i=0;i<3;++i)
			ftm.c[i]+=pos[i];					// parent frame center plus self position
		quat q={1,0,0,0};						// init a conventional oriented quat
		ftm=q.ai16(tpb);						// turn, pitch and bank frame's matrix
		ak.c=ftm.c;								// update new animation center
		bool of=true;							// false will merge with nested frame
		if(p&&p->mnm&&frIds.size()==1&&mhId=="null"){	// true if merge frames of null mesh
			vertex vpos;
			vpos=ftm.c;
			of=vpos.anyChange();
			frame&f=p->frs.frMap[frIds[0]];
			vpos=f.ftm.c;
			of=of||vpos.anyChange();
			typedef vector<af3>::iterator it_vP;
			each(it_vP,ak.poss){
				vpos=*it_vP;
				of=of||vpos.anyChange();
			}
			if(!of){
				cout<<name<<" will be merged to "
					<<frIds[0]<<":\t"<<cnt<<endl;
				vector<string>&s=f.ak.parents;
				s=ak.parents;
				s.push_back(name);
			}
		}
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		if(of)ss<<"Frame "<<name<<"{\n";
		if(ftm.anyChange()&&of)					// true when new center/rotation
			ss<<ftm<<endl;						// output
		if(p){									// true when persistent frame collected
			if(of)p->aks<<ak;					// save current animkey
			mesh&mh=p->mhs.mhMap[mhId],cmh(mh);	// shorcut to mesh, clone mesh
			if(mh.name!="null"){				// true when there is mesh
				if(mh.pcnt&&(*mh.pcnt!=cnt)){	// true when different cnt
					string s="C~";				// default clone mesh prefix
					s+=cmh.name;				// concatenate name
					if(cmh.name[1]=='~')cmh.name[0]+=1;	// true if cloned before
					else cmh.name=s;			// else need new name
					p->mhs<<cmh;				// save clone mesh
					mhId=cmh.name;				// output updated clone id instead
					cout<<"different center in Frame: "<<name<<" mesh: "<<cmh.name
						<<"\nbefore: "<<*mh.pcnt
						<<"\naffter: "<<cnt<<"\n\n";
				}
				p->mhs.mhMap[mhId].pcnt=&cnt;	// update center before output
			}
			if(of)if(p->nstmh)						// true when output nested mesh
				ss<<p->mhs.mhMap[mhId]<<endl;
			else{
				p->omhs<<cmh;					// output mesh definition
				ss<<'{'<<mhId<<"}\n";			// output mesh id only
			}
		}
		else ss<<'{'<<mhId<<"}\n";
		each(it_vs_,frIds)						// iterate each nested frame id
 			if(p){								// true when output nested frame
				frame&f=p->frs.frMap[*it_vs_];	// shortcut to current nested frame
				if(f.name!=""){					// true if not blacklisted
					af3 cmod={-cnt.x,-cnt.y,-cnt.z};
					f.ftm=cmod;					// update new center
					ss<<f<<endl;
				}
			}
 			else ss<<'{'<<*it_vs_<<"}\n";		// last nested frame id
		if(of)ss<<"}";
		RETURN_CONST_C_STR(ss);
		
	}
}frame;
typedef map<string,material>::const_iterator itsMT_;
typedef struct SMapCollMat{
	map<string,material>mtMap;
	SMapCollMat&operator<<(const material&m){
		if(m.name!="")mtMap[m.name]=m;
		return*this;
	}
	SMapCollMat&operator>>(material&m){
		itsMT it=mtMap.begin();
		m=it->second;
		mtMap.erase(it);
		return*this;
	}
	SMapCollMat&operator<<(const mlist&m){
		each(itsMT_,mtMap)
			mtMap[itsMT_->first]=itsMT_->second;
		return*this;
	}
	SMapCollMat&operator>>(mlist&m){
		m.mtMap=mtMap;
		mtMap.clear();
		return*this;
	}
	operator cstr(){
		stringstream ss;
		each(itsMT,mtMap)
			ss<<itsMT->second<<endl;
		ss<<endl;
		RETURN_CONST_C_STR(ss);
	}
}collMat;
typedef map<string,mesh>::iterator itsMH;
friend struct SMapCollMsh;
typedef struct SMapCollMsh{
	SMapCollMsh(CDnmConvX&c):p(c){};
	CDnmConvX&p;
	map<string,mesh>mhMap;
	SMapCollMsh&operator<<(const mesh&m){
		if(m.name!="")mhMap[m.name]=m;
		return*this;
	}
	SMapCollMsh&operator>>(mesh&m){
		itsMH it=mhMap.begin();
		m=it->second;
		mhMap.erase(it);
		return*this;
	}
	operator cstr(){
		if(p.mhbl.size()&&mhMap.size())				// true when mesh blacklist from ini
			each(it_vs_,p.mhbl)
				mhMap[*it_vs_].clear();
		stringstream ss;
		each(itsMH,mhMap)
			if(itsMH->first!="")
				ss<<itsMH->second<<endl;
		ss<<endl;
		RETURN_CONST_C_STR(ss);
	}
}collMsh;
typedef map<string,frame>::iterator itsFR;
friend struct SMapCollFrm;
typedef struct SMapCollFrm{
	SMapCollFrm(CDnmConvX&c):p(c){};
	CDnmConvX&p;
	map<string,frame>frMap;
	SMapCollFrm&operator<<(const frame&f){
		if(f.name!=""){
			frame&fr=frMap[f.name];				// find where to save
			fr=f;								// save the frame
		}
		return*this;
	}
	SMapCollFrm&operator>>(frame&f){
		itsFR it=frMap.begin();
		f=it->second;
		frMap.erase(it);
		return*this;
	}
	operator cstr(){
		each(it_vs_,p.invfidx){					// invert face by idx
			istringstream is(*it_vs_);
			string s;							// get mesh name
			u16 i;								// get face index to invert
			is>>s>>i;
			p.mhs.mhMap[s].invertFace(i);
		}
		each(it_vs_,p.invfmt){					// invert face by mesh
			istringstream is(*it_vs_);
			string s,mt;
			is>>s>>mt;
			p.mhs.mhMap[s].invertFace(mt);
		}
		map<string,mesh>&mhMap=p.mhs.mhMap;
		each(it_vs_,p.mhbl){					// blacklist mesh
			itsMH it=mhMap.find(*it_vs_);
			if(it!=mhMap.end())
				it->second.clear();
			else cout<<"Blacklist Mesh not found: "<<*it_vs_<<endl;
		}
		each(it_vs_,p.frbl){					// blacklist frame
			itsFR it=frMap.find(*it_vs_);
			if(it!=frMap.end())
				it->second.clear();
			else cout<<"Blacklist Frame not found: "<<*it_vs_<<endl;
		}
		stringstream ss;
		each(itsFR,frMap)						// loop to find main parent frames
			if(!itsFR->second.nested&&itsFR->second.name!="")	// true if not nested and not empty
				ss<<itsFR->second<<endl;		// output data
		ss<<endl;
		RETURN_CONST_C_STR(ss);
	}
}collFrm;
typedef map<string,anikey>::iterator itsA;
friend struct SMapCollAnim;
typedef struct SMapCollAnim{
	SMapCollAnim(CDnmConvX&c):p(c){};
	CDnmConvX&p;
	map<string,anikey>akMap;					// AnimationSet{
	SMapCollAnim&operator<<(anikey&ak){
		akMap[ak.name]=ak;
		return*this;
	}
	operator cstr(){
		stringstream ss;
		ss<<FLOAT_PRECISION(6);
		ss<<"AnimationSet{\n";
		each(itsA,akMap)
			if(itsA->second.name!="")					// true if not blacklisted
				ss<<"Animation{\n{"<<itsA->first<<"}\n"	// each frame name
					<<itsA->second						// frame's AnimationKey
					<<"\nAnimationOptions{0;0;}\n}\n";	// footer
		ss<<"}\n";
		RETURN_CONST_C_STR(ss);
	}
}collAni;

private:
	string inFilePath;
	collMat mts,omts;							// material collector and output
	collMsh mhs,omhs;							// mesh collector and output
	collFrm frs;								// frame collector
	collAni aks;								// animkey collector
	bool nstmt;									// use nested material config
	bool nstmh;									// use nested mesh config
	bool mnm;									// merge frame/animkey of null mesh
	vector<string>configs;						// general config
	vector<string>mhbl;							// blacklist mesh
	vector<string>frbl;							// blacklist frame
	vector<string>dsmh;							// double side mesh
	vector<string>invfidx;						// invert face by idx
	vector<string>invfmt;						// invert face by material
	map<u16,map<u16,u16>>otl;					// keyframes<cla,sta> relationship
};

// manipulator to skip any char
template<char C>
std::istream&skip(std::istream&is){
	if((is>>std::ws).peek()==C)is.ignore();
	else is.setstate(std::ios_base::failbit);
	return is;
}// i.e.  istr>>skip<'#'>;
#pragma warning(pop)							// end of disableSpecificWarnings
#endif
