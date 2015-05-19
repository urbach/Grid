//---------------------------------------------------------------------------
/*! @file Grid_vector_types.h
  @brief Defines templated class Grid_simd to deal with inner vector types
*/
// Time-stamp: <2015-05-19 17:20:36 neo>
//---------------------------------------------------------------------------
#ifndef GRID_VECTOR_TYPES
#define GRID_VECTOR_TYPES

#include "Grid_sse4.h"


namespace Grid {

  // To take the floating point type of real/complex type
  template <typename T> 
    struct RealPart {
      typedef T type;
    };
  template <typename T> 
    struct RealPart< std::complex<T> >{
    typedef T type;
  };

  ////////////////////////////////////////////////////////
  // Check for complexity with type traits
  template <typename T> 
    struct is_complex : std::false_type {};
  template < typename T > 
    struct is_complex< std::complex<T> >: std::true_type {};
  ////////////////////////////////////////////////////////
  // Define the operation templates functors
  // general forms to allow for vsplat syntax
  // need explicit declaration of types when used since
  // clang cannot automatically determine the output type sometimes
  template < class Out, class Input1, class Input2, class Operation > 
    Out binary(Input1 src_1, Input2 src_2, Operation op){
    return op(src_1, src_2);
  } 

  template < class SIMDout, class Input, class Operation > 
    SIMDout unary(Input src, Operation op){
    return op(src);
  } 

  ///////////////////////////////////////////////

  /*
    @brief Grid_simd class for the SIMD vector type operations
   */
  template < class Scalar_type, class Vector_type > 
    class Grid_simd {
    
  public:
    typedef typename RealPart < Scalar_type >::type Real; 
    Vector_type v;
    

    static inline int Nsimd(void) { return sizeof(Vector_type)/sizeof(Scalar_type);}

    // Constructors
    Grid_simd & operator = ( Zero & z){
      vzero(*this);
      return (*this);
    }
    Grid_simd(){};
    
    
    //Enable if complex type
    template < class S = Scalar_type > 
    Grid_simd(typename std::enable_if< is_complex < S >::value, S>::type a){
      vsplat(*this,a);
    };
    

    Grid_simd(Real a){
      vsplat(*this,Scalar_type(a));
    };


       
    ///////////////////////////////////////////////
    // mac, mult, sub, add, adj
    ///////////////////////////////////////////////
    friend inline void mac (Grid_simd * __restrict__ y,const Grid_simd * __restrict__ a,const Grid_simd *__restrict__ x){ *y = (*a)*(*x)+(*y); };
    friend inline void mult(Grid_simd * __restrict__ y,const Grid_simd * __restrict__ l,const Grid_simd *__restrict__ r){ *y = (*l) * (*r); }
    friend inline void sub (Grid_simd * __restrict__ y,const Grid_simd * __restrict__ l,const Grid_simd *__restrict__ r){ *y = (*l) - (*r); }
    friend inline void add (Grid_simd * __restrict__ y,const Grid_simd * __restrict__ l,const Grid_simd *__restrict__ r){ *y = (*l) + (*r); }
    //not for integer types... FIXME
    friend inline Grid_simd adj(const Grid_simd &in){ return conj(in); }
        
    ///////////////////////////////////////////////
    // Initialise to 1,0,i for the correct types
    ///////////////////////////////////////////////
    // if not complex overload here 
    template <  class S = Scalar_type,typename std::enable_if < !is_complex < S >::value, int >::type = 0 > 
      friend inline void vone(Grid_simd &ret)      { vsplat(ret,1.0); }
    template <  class S = Scalar_type,typename std::enable_if < !is_complex < S >::value, int >::type = 0 > 
      friend inline void vzero(Grid_simd &ret)     { vsplat(ret,0.0); }
    
    // overload for complex type
    template <  class S = Scalar_type,typename std::enable_if < is_complex < S >::value, int >::type = 0 > 
      friend inline void vone(Grid_simd &ret)      { vsplat(ret,1.0,0.0); }
    template < class S = Scalar_type,typename std::enable_if < is_complex < S >::value, int >::type = 0 > 
      friend inline void vzero(Grid_simd &ret)     { vsplat(ret,0.0,0.0); }// use xor?
    
    // For integral type
    template <  class S = Scalar_type,typename std::enable_if < std::is_integral < S >::value, int >::type = 0 > 
      friend inline void vone(Grid_simd &ret)      { vsplat(ret,1); }
    template <  class S = Scalar_type,typename std::enable_if < std::is_integral < S >::value, int >::type = 0 > 
      friend inline void vzero(Grid_simd &ret)      { vsplat(ret,0); }
    template <  class S = Scalar_type,typename std::enable_if < std::is_integral < S >::value, int >::type = 0 > 
      friend inline void vtrue (Grid_simd &ret){vsplat(ret,0xFFFFFFFF);}
    template <  class S = Scalar_type,typename std::enable_if < std::is_integral < S >::value, int >::type = 0 > 
      friend inline void vfalse(vInteger &ret){vsplat(ret,0);}


    // do not compile if real or integer, send an error message from the compiler
    template < class S = Scalar_type,typename std::enable_if < is_complex < S >::value, int >::type = 0 > 
    friend inline void vcomplex_i(Grid_simd &ret){ vsplat(ret,0.0,1.0);}
   
    ////////////////////////////////////
    // Arithmetic operator overloads +,-,*
    ////////////////////////////////////
    friend inline Grid_simd operator + (Grid_simd a, Grid_simd b)
    {
      Grid_simd ret;
      ret.v = binary<Vector_type>(a.v, b.v, SumSIMD());
      return ret;
    };
        
    friend inline Grid_simd operator - (Grid_simd a, Grid_simd b)
    {
      Grid_simd ret;
      ret.v = binary<Vector_type>(a.v, b.v, SubSIMD());
      return ret;
    };
        
    // Distinguish between complex types and others
    template < class S = Scalar_type, typename std::enable_if < is_complex < S >::value, int >::type = 0 >
      friend inline Grid_simd operator * (Grid_simd a, Grid_simd b)
      {
	Grid_simd ret;
	ret.v = binary<Vector_type>(a.v,b.v, MultComplexSIMD());
	return ret;
      };

    // Real/Integer types
    template <  class S = Scalar_type,typename std::enable_if < !is_complex < S >::value, int >::type = 0 > 
    friend inline Grid_simd operator * (Grid_simd a, Grid_simd b)
      {
	Grid_simd ret;
	ret.v = binary<Vector_type>(a.v,b.v, MultSIMD());
	return ret;
      };
    



    ////////////////////////////////////////////////////////////////////////
    // FIXME:  gonna remove these load/store, get, set, prefetch
    ////////////////////////////////////////////////////////////////////////
    friend inline void vset(Grid_simd &ret, Scalar_type *a){
      ret.v = unary<Vector_type>(a, VsetSIMD());
    }
        
    ///////////////////////
    // Splat
    ///////////////////////
    // overload if complex
    template < class S = Scalar_type > 
    friend inline void vsplat(Grid_simd &ret, typename std::enable_if< is_complex < S >::value, S>::type c){
      Real a = real(c);
      Real b = imag(c);
      vsplat(ret,a,b);
    }

    // this only for the complex version
    template < class S = Scalar_type, typename std::enable_if < is_complex < S >::value, int >::type = 0 > 
    friend inline void vsplat(Grid_simd &ret,Real a, Real b){
      ret.v = binary<Vector_type>(a, b, VsplatSIMD());
    }    

    //if real fill with a, if complex fill with a in the real part (first function above)
    friend inline void vsplat(Grid_simd &ret,Real a){
      ret.v = unary<Vector_type>(a, VsplatSIMD());
    }    


    friend inline void vstore(const Grid_simd &ret, Scalar_type *a){
      binary<void>(ret.v, (Real*)a, VstoreSIMD());
    }

    friend inline void vprefetch(const Grid_simd &v)
    {
      _mm_prefetch((const char*)&v.v,_MM_HINT_T0);
    }


    friend inline Scalar_type Reduce(const Grid_simd & in)
    {
      // FIXME add operator
    }

    friend inline Grid_simd operator * (const Scalar_type &a, Grid_simd b){
      Grid_simd va;
      vsplat(va,a);
      return va*b;
    }
    friend inline Grid_simd operator * (Grid_simd b,const Scalar_type &a){
      return a*b;
    }

    ///////////////////////
    // Conjugate
    ///////////////////////
								     
    friend inline Grid_simd  conj(const Grid_simd  &in){
      Grid_simd  ret ; vzero(ret);
      // FIXME add operator
      return ret;
    }
    friend inline Grid_simd timesMinusI(const Grid_simd &in){
      Grid_simd ret; 
      vzero(ret);
      // FIXME add operator
      return ret;
    }
    friend inline Grid_simd timesI(const Grid_simd &in){
      Grid_simd ret; vzero(ret);
      // FIXME add operator
      return ret;
    }
        
    // Unary negation
    friend inline Grid_simd operator -(const Grid_simd &r) {
      vComplexF ret;
      vzero(ret);
      ret = ret - r;
      return ret;
    }
    // *=,+=,-= operators
    inline Grid_simd &operator *=(const Grid_simd &r) {
      *this = (*this)*r;
      return *this;
      // return (*this)*r; ?
    }
    inline Grid_simd &operator +=(const Grid_simd &r) {
      *this = *this+r;
      return *this;
    }
    inline Grid_simd &operator -=(const Grid_simd &r) {
      *this = *this-r;
      return *this;
    }



     friend inline void permute(Grid_simd &y,Grid_simd b,int perm)
      {
        Gpermute<Grid_simd>(y,b,perm);
      }

     /*
    friend inline void permute(Grid_simd &y,Grid_simd b,int perm)
    {
      Gpermute<Grid_simd>(y,b,perm);
    }
    friend inline void merge(Grid_simd &y,std::vector<Scalar_type *> &extracted)
    {
      Gmerge<Grid_simd,Scalar_type >(y,extracted);
    }
    friend inline void extract(const Grid_simd &y,std::vector<Scalar_type *> &extracted)
    {
      Gextract<Grid_simd,Scalar_type>(y,extracted);
    }
    friend inline void merge(Grid_simd &y,std::vector<Scalar_type > &extracted)
    {
      Gmerge<Grid_simd,Scalar_type >(y,extracted);
    }
    friend inline void extract(const Grid_simd &y,std::vector<Scalar_type > &extracted)
    {
      Gextract<Grid_simd,Scalar_type>(y,extracted);
    }
     */

  };// end of Grid_simd class definition 






  template<class scalar_type, class vector_type > 
    inline Grid_simd< scalar_type, vector_type>  innerProduct(const Grid_simd< scalar_type, vector_type> & l, const Grid_simd< scalar_type, vector_type> & r) 
  {
    return conj(l)*r; 
  }

  template<class scalar_type, class vector_type >
  inline void zeroit(Grid_simd< scalar_type, vector_type> &z){ vzero(z);}


  template<class scalar_type, class vector_type >
  inline Grid_simd< scalar_type, vector_type> outerProduct(const Grid_simd< scalar_type, vector_type> &l, const Grid_simd< scalar_type, vector_type>& r)
  {
    return l*r;
  }


  template<class scalar_type, class vector_type >
  inline Grid_simd< scalar_type, vector_type> trace(const Grid_simd< scalar_type, vector_type> &arg){
    return arg;
  }


  // Define available types (now change names to avoid clashing)

  typedef Grid_simd< float                 , SIMD_Ftype > MyRealF;
  typedef Grid_simd< double                , SIMD_Dtype > MyRealD;
  typedef Grid_simd< std::complex< float > , SIMD_Ftype > MyComplexF;
  typedef Grid_simd< std::complex< double >, SIMD_Dtype > MyComplexD;



}

#endif