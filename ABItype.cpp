/** \file
    \brief Contains the ABItype class, as well as its TFLAG helper class
*/
#include "ABItype.h"

/** \brief Constructor
*/
ABItype::ABItype ()
    {
    }

/** \brief Destructor, deletes flags and their associated data
*/
ABItype::~ABItype ()
    {
    for ( unsigned int a = 0 ; a < vf.size() ; a++ )
        if ( vf[a].data ) delete vf[a].data ;
    vf.clear () ;
    }

/** \brief Checks for a CMBF structure from the ABI data, returns its length
    \param t Pointer to the data to parse
    \param l Length of the data
*/
int ABItype::getCMBF ( const unsigned char * const t, const int l ) const
    {
    const unsigned char *r = NULL ;
    for ( const unsigned char *s = t + l - 8 ; s > t ; s-- )
       {
       if ( *s == 'C' )
          {
          if ( *(s+1) == 'M' &&
               *(s+2) == 'B' &&
               *(s+3) == 'F' )
             {
             r = s ;
             break ;
             }
          }
       }
    if ( !r ) return -1 ;
    // difference between two pointers that should be positive
    return (int) (r - t) ;
    }

/** \brief Parses an ABI format file
    \param filename The filename. Surprise!

    The "parsing" works by
     * - detecting and ignoring the Mac file header, if any
     * - finding the offset for the first CMBF structure
     * - iterating through the CMBF structures
*/
void ABItype::parse ( const wxString& filename ) /* not const */
    {
    wxFile f ( filename , wxFile::read ) ;
    long l = f.Length() ;
    unsigned char *t = new unsigned char [l+15] ;
    f.Read ( t , l ) ;
    f.Close() ;

    // Parsing "t"
    int macOffset = getMacOffset ( t ) ; // Trying to compensate for MAC header
    if ( macOffset == -1 ) return ; // No valid ABI file
    vf.clear () ;

    int pos = macOffset ;
    pos += 4 ; // ABIF
    pos += 2 ; // Unknown
    pos += 4 ; // tdir
    pos += 4 ; // Instance = 1
    pos += 2 ; // Datatype
    pos += 2 ; // Datasize
    int nrecords = getInt4 ( t , pos ) ;
    pos += 4 ; // # Bytes
    int cnt = getInt4 ( t , pos ) + macOffset ; // Pointer

    while ( nrecords > 0 )
        {
        TFLAG flag = getFlag ( t , cnt ) ;
        if ( flag.nbytes > 0 )
           {
           if ( flag.nbytes > 4 )
              {
              flag.data = new unsigned char [ flag.nbytes+5 ] ;
              for ( int i = 0 ; i < flag.nbytes ; i++ )
                 flag.data[i] = t[macOffset+flag.value+i] ;
              }
           vf.push_back ( flag ) ;
           }
        nrecords-- ;
        }

    delete [] t ;
    }

/** \brief Deterimnes the position of the "ABIF" key
    \param t Pointer to the raw data

    This returns
     * - 0 for a valid Windows file
     * - 128 for a valid Mac file
     * - -1 if the file is invalid
*/
int ABItype::getMacOffset ( const unsigned char * const t ) const
    {
    int r = 0 ;
    if ( t[r+0]=='A' && t[r+1]=='B' && t[r+2]=='I' && t[r+3]=='F' ) return r ;
    r = 128 ;
    if ( t[r+0]=='A' && t[r+1]=='B' && t[r+2]=='I' && t[r+3]=='F' ) return r ;
    return -1 ;
    }

/** \brief Sets a TFLAG structure from the data
    \param t Pointer to the raw data
    \param from Offset; is changed after the structure is read
*/
TFLAG ABItype::getFlag ( const unsigned char * const t , int &from ) const
    {
    TFLAG r ;
    r.data = NULL ;
    r.pos = from ;
    r.flag = getStr ( t , from , 4 ) ; from += 4 ;
    r.instance = getInt4 ( t , from ) ;
    r.datatype = getInt2 ( t , from ) ;
    r.datasize = getInt2 ( t , from ) ;
    r.nrecords = getInt4 ( t , from ) ;
    r.nbytes = getInt4 ( t , from ) ;
    r.value = getInt4 ( t , from ) ;
    r.spare = getInt4 ( t , from ) ;
    r.after = from ;
    return r ;
    }

wxString ABItype::getText ( const unsigned char * const t , int &from ) const
    {
    wxString r ;
    int l = t[from++] ;
    for ( int a = 0 ; a < l ; a++ ) r += t[from++] ;
    return r ;
    }

// This function does not appear to be used anywhere...
int ABItype::getInt10 ( const unsigned char * const t , int &from ) const
    {
    from += 10 ; // Essentially ignoring
    return 0 ;
    }

int ABItype::getInt1 ( const unsigned char * const t , int &from ) const
    {
    return (unsigned char) t[from++] ;
    }

int ABItype::getInt2 ( const unsigned char * const t , int &from ) const
    {
    int t1 = (unsigned char) t[from++] ;
    int t2 = (unsigned char) t[from++] ;
    int r = t2 + ( t1 << 8 ) ;
    return r ;
    }

int ABItype::getInt4 ( const unsigned char * const t , int &from ) const
    {
    int t1 = getInt2 ( t , from ) ;
    int t2 = getInt2 ( t , from ) ;
    int r = ( t1 << 16 ) + t2 ;
    return r ;
    }

/** \brief Creates a new wxString as a substring of string t of len characters length starting from position from
 * \param t The string to extract from
 * \param from The position to start from, 0-based
 * \param len The length of the substring
 * \return The substring
 * \note This function is not used anywhere in the code and FIXME: it could be otimized in various ways.
*/
wxString ABItype::getStr ( const unsigned char * const t , const int from , const int len ) const
    {
    wxString r ;
    for ( int a = 0 ; a < len ; a++ ) r += t[from+a] ;
    return r ;
    }

//--------------------------------------------


/** \brief Finds a specific record in the parsed data
    \param id The flag (type) of the record
    \param num The number of the record
*/
int ABItype::getRecord ( const wxString& id , const int num ) const
    {
    unsigned int a ;
    for ( a = 0 ; a < vf.size() && ( vf[a].flag != id || vf[a].instance != num ) ; a++ ) ;
    if ( a == vf.size() ) return -1 ; // Not found
    return a ;
    }

/** \brief Finds a specific sequence record
    \param num The number of the sequence record
*/
wxString ABItype::getSequence ( const int num ) const
    {
    wxString r ;
    int a = getRecord ( _T("PBAS") , num ) ;
    myass ( a > -1 , _T("ABItype::getSequence") ) ;
    for ( int b = 0 ; b < vf[a].nbytes ; b++ ) r += vf[a].data[b] ;
    return r ;
    }

/** \brief Returns the string value of a specific record in the parsed data
    \param id The flag (type) of the record
    \param num The number of the record
*/
wxString ABItype::getRecordPascalString ( const wxString& id , const int num ) const
    {
    int i = getRecord ( id , num ) ;
    if ( i == -1 ) return _T("") ;
    return vf[i].getPascalString() ;
    }

/** \brief Returns the (integer) value of a specific record in the parsed data
    \param id The flag (type) of the record
    \param num The number of the record
*/
int ABItype::getRecordValue ( const wxString& id , const int num ) const
    {
    int i = getRecord ( id , num ) ;
    if ( i == -1 ) return 0 ;
    return vf[i].value ;
    }


//***************************************

/// Constructor
TFLAG::TFLAG ()
    {
    }

/// Destructor
TFLAG::~TFLAG ()
    {
    }

/// \brief Reads a Pascal-like string from the data
wxString TFLAG::getPascalString () const
    {
    if ( !data )
       {
       char t[5] ;
//     t[0] = ( value >> 24 ) & 255 ;
       t[1] = ( value >> 16 ) & 255 ;
       t[2] = ( value >>  8 ) & 255 ;
       t[3] = ( value >>  0 ) & 255 ;
       t[4] = 0 ;
       return wxString ( t+1 , wxConvUTF8 ) ;
       }
    wxString r ;
    const int len = (unsigned char) data[0] ;
    for ( int a = 1 ; a <= len ; a++ ) r += data[a] ;
    return r ;
    }

