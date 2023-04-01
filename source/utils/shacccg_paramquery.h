// Taken from https://github.com/SonicMastr/Pigs-In-A-Blanket/blob/master/include/shacccg/paramquery.h
#ifndef _DOLCESDK_PSP2_SHACCCG_PARAMQUERY_H_
#define _DOLCESDK_PSP2_SHACCCG_PARAMQUERY_H_

#include <stdint.h>

#ifdef	__cplusplus
extern "C" {
#endif	// def __cplusplus

///////////////////////////////////////////////////////////////////////////////
// Forward declarations
///////////////////////////////////////////////////////////////////////////////
typedef void const * SceShaccCgParameter;
typedef struct SceShaccCgCompileOutput SceShaccCgCompileOutput;

///////////////////////////////////////////////////////////////////////////////
// Constants
///////////////////////////////////////////////////////////////////////////////

/** @brief	Classifies shader parameter class

	@ingroup shacccg
*/
typedef enum SceShaccCgParameterClass {
	SCE_SHACCCG_PARAMETERCLASS_INVALID		= 0x00,	///< An invalid parameter class.
	SCE_SHACCCG_PARAMETERCLASS_SCALAR		= 0x01,	///< Scalar parameter class.
	SCE_SHACCCG_PARAMETERCLASS_VECTOR		= 0x02,	///< Vector parameter class.
	SCE_SHACCCG_PARAMETERCLASS_MATRIX		= 0x03,	///< Matrix parameter class.
	SCE_SHACCCG_PARAMETERCLASS_STRUCT		= 0x04,	///< Struct parameter class.
	SCE_SHACCCG_PARAMETERCLASS_ARRAY		= 0x05,	///< Array parameter class.
	SCE_SHACCCG_PARAMETERCLASS_SAMPLER		= 0x06,	///< Sampler parameter class.
	SCE_SHACCCG_PARAMETERCLASS_UNIFORMBLOCK	= 0x07	///< Uniform Block parameter class.
} SceShaccCgParameterClass;

/** @brief	Classifies shader parameter data format

	@ingroup shacccg
*/
typedef enum SceShaccCgParameterBaseType {
	SCE_SHACCCG_BASETYPE_INVALID		= 0x00,		///< An invalid format.
	SCE_SHACCCG_BASETYPE_FLOAT			= 0x01,		///< Full precision 32-bit floating point.
	SCE_SHACCCG_BASETYPE_HALF			= 0x02,		///< Half precision 16-bit floating point.
	SCE_SHACCCG_BASETYPE_FIXED			= 0x03,		///< 2.8 fixed point precision.
	SCE_SHACCCG_BASETYPE_BOOL			= 0x04,		///< Boolean value.
	SCE_SHACCCG_BASETYPE_CHAR			= 0x05,		///< Signed char (8-bit) value.
	SCE_SHACCCG_BASETYPE_UCHAR			= 0x06,		///< Unsigned char (8-bit) value.
	SCE_SHACCCG_BASETYPE_SHORT			= 0x07,		///< Signed short (16-bit) value.
	SCE_SHACCCG_BASETYPE_USHORT			= 0x08,		///< Unsigned short (16-bit) value.
	SCE_SHACCCG_BASETYPE_INT			= 0x09,		///< Signed int (32-bit) value.
	SCE_SHACCCG_BASETYPE_UINT			= 0x0a,		///< Unsigned int (32-bit) value.
	SCE_SHACCCG_BASETYPE_SAMPLER1D		= 0x0b,		///< 1D sampler
	SCE_SHACCCG_BASETYPE_ISAMPLER1D		= 0x0c,		///< 1D signed integer sampler
	SCE_SHACCCG_BASETYPE_USAMPLER1D		= 0x0d,		///< 1D unsigned integer sampler
	SCE_SHACCCG_BASETYPE_SAMPLER2D		= 0x0e,		///< 2D sampler
	SCE_SHACCCG_BASETYPE_ISAMPLER2D		= 0x0f,		///< 2D signed integer sampler
	SCE_SHACCCG_BASETYPE_USAMPLER2D		= 0x10,		///< 2D unsigned integer sampler
	SCE_SHACCCG_BASETYPE_SAMPLERCUBE	= 0x11,		///< Cube sampler
	SCE_SHACCCG_BASETYPE_ISAMPLERCUBE	= 0x12,		///< Cube signed integer sampler
	SCE_SHACCCG_BASETYPE_USAMPLERCUBE	= 0x13,		///< Cube unsigned integer sampler
	SCE_SHACCCG_BASETYPE_ARRAY			= 0x17,		///< An array
	SCE_SHACCCG_BASETYPE_STRUCT			= 0x18,		///< A structure
	SCE_SHACCCG_BASETYPE_UNIFORMBLOCK	= 0x19		///< A uniform block
} SceShaccCgParameterBaseType;


///////////////////////////////////////////////////////////////////////////////
// Structs
///////////////////////////////////////////////////////////////////////////////

/** @brief	Classifies matrix memory layout

	@ingroup shacccg
*/
typedef enum SceShaccCgParameterMemoryLayout
{
	SCE_SHACCCG_MEMORYLAYOUT_INVALID,				///< Invalid memory layout
	SCE_SHACCCG_MEMORYLAYOUT_COLUMN_MAJOR,			///< Column major memory layout
	SCE_SHACCCG_MEMORYLAYOUT_ROW_MAJOR				///< Row major memory layout
} SceShaccCgParameterMemoryLayout;

/** @brief	Classifies shader parameter variability

	@ingroup shacccg
*/
typedef enum SceShaccCgParameterVariability
{
	SCE_SHACCCG_VARIABILITY_INVALID,				///< Invalid variability
	SCE_SHACCCG_VARIABILITY_VARYING,				///< Parameter is varying
	SCE_SHACCCG_VARIABILITY_UNIFORM					///< Parameter is uniform
} SceShaccCgParameterVariability;

/** @brief	Classifies shader parameter direction

	@ingroup shacccg
*/
typedef enum SceShaccCgParameterDirection
{
	SCE_SHACCCG_DIRECTION_INVALID,					///< Invalid direction
	SCE_SHACCCG_DIRECTION_IN,						///< Parameter is input
	SCE_SHACCCG_DIRECTION_OUT						///< Parameter is output
} SceShaccCgParameterDirection;


///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

/**	@brief	Start parameter enumeration.

	Start parameter enumeration.

	@param[in] prog
		The output of a successful shader compilation.

	@return
		A SceShaccCgParameter object representing the first parameter in the shader.
		If 0 is returned, the input argument was malformed or the shader has no public
		symbols.

	@ingroup shacccg
*/
SceShaccCgParameter sceShaccCgGetFirstParameter(
	SceShaccCgCompileOutput const* prog);

/**	@brief	Access the next parameter in the global list of shader parameter.

	Access the next parameter in the global list of shader parameter.

	@param[in] param
		The current parameter object.

	@return
		A SceShaccCgParameter object representing the next parameter, or NULL if there are
		no more parameters.

	@ingroup shacccg
*/
SceShaccCgParameter sceShaccCgGetNextParameter(
	SceShaccCgParameter param);

/**	@brief	Find a parameter by its name.

	Find a parameter by its name.

	@param[in] prog
		The output of a successful shader compilation.

	@param[in] name
		The name of the parameter.

	@return
		A SceShaccCgParameter object representing the parameter or NULL if the
		parameter was not found.

	@ingroup shacccg
*/
SceShaccCgParameter sceShaccCgGetParameterByName(
	SceShaccCgCompileOutput const* prog,
	char const *name);

/**	@brief	Returns the name of a parameter.

	Returns the name of a parameter.

	@param[in] param
		The parameter object.

	@return
		A NULL terminated string containing the name of the parameter

	@ingroup shacccg
*/
const char * sceShaccCgGetParameterName(
	SceShaccCgParameter param);

/**	@brief	Returns the semantic of a parameter.

	Returns the semantic of a parameter.

	@param[in] param
		The parameter object.

	@return
		A NULL terminated string containing the semantic of the parameter or NULL
		if no semantic was declared

	@ingroup shacccg
*/
const char * sceShaccCgGetParameterSemantic(
	SceShaccCgParameter param);

/**	@brief	Returns the user declared type of a parameter.

	Returns the user declared type of a parameter.

	@param[in] param
		The parameter object.

	@return
		A NULL terminated string containing the user declared type of the parameter or NULL
		if no user declared type was used

	@ingroup shacccg
*/
const char * sceShaccCgGetParameterUserType(
	SceShaccCgParameter param);

/**	@brief	Returns the parameter class.

	Returns the parameter class.

	@param[in] param
		The parameter object.

	@return
		The SceShaccCgParameterClass value this parameter is part of.

	@ingroup shacccg
*/
SceShaccCgParameterClass sceShaccCgGetParameterClass(
	SceShaccCgParameter param);

/**	@brief	Returns the parameter variability.

	Returns the parameter variability.

	@param[in] param
		The parameter object.

	@return
		The SceShaccCgParameterVariability value for the parameter.

	@ingroup shacccg
*/
SceShaccCgParameterVariability sceShaccCgGetParameterVariability(
	SceShaccCgParameter param);

/**	@brief	Returns the parameter direction.

	Returns the parameter direction.

	@param[in] param
		The parameter object.

	@return
		The SceShaccCgParameterDirection value for the parameter.

	@ingroup shacccg
*/
SceShaccCgParameterDirection sceShaccCgGetParameterDirection(
	SceShaccCgParameter param);

/**	@brief	Returns the parameter base type.

	Returns the parameter base type.

	@param[in] param
		The parameter object.

	@return
		The SceShaccCgParameterBaseType value for the parameter.

	@ingroup shacccg
*/
SceShaccCgParameterBaseType sceShaccCgGetParameterBaseType(
	SceShaccCgParameter param);

/**	@brief	Returns true if the parameter is referenced.

	Returns true if the parameter is referenced.

	@param[in] param
		The parameter object.

	@return
		1 if the value is referenced, otherwise return 0 if the parameter is dead.

	@ingroup shacccg
*/
int32_t sceShaccCgIsParameterReferenced(
	SceShaccCgParameter param);

/**	@brief	Returns the hw resource index of the parameter.

	Returns the hw resource index of the parameter.

	@param[in] param
		The parameter object.

	@return
		The resource index or a value of -1 if no resource is assigned to this parameter.

	@ingroup shacccg
*/
uint32_t sceShaccCgGetParameterResourceIndex(
	SceShaccCgParameter param);

/**	@brief	Returns the buffer index of the parameter.

	Returns the buffer index of the parameter.

	@param[in] param
		The parameter object.

	@return
		The buffer index or a value of -1 if no buffer is assigned to this parameter.

	@ingroup shacccg
*/
uint32_t sceShaccCgGetParameterBufferIndex(
	SceShaccCgParameter param);

/**	@brief	Returns true if the parameter is __regformat.

	Returns true if the parameter is __regformat.

	@param[in] param
		The parameter object.

	@return
		1 if the value is __regformat, otherwise return 0.

	@ingroup shacccg
*/
int32_t sceShaccCgIsParameterRegFormat(
	SceShaccCgParameter param);

/**	@brief	Returns the first member for a struct parameter.

	Returns the first member for a struct parameter.

	@param[in] param
		The parameter object.

	@return
		The parameter object for the first member of a struct or NULL if the parameter was
		malformed.

	@ingroup shacccg
*/
SceShaccCgParameter sceShaccCgGetFirstStructParameter(
	SceShaccCgParameter param);

/**	@brief	Returns the first member for a uniform block parameter.

	Returns the first member for a uniform block parameter.

	@param[in] param
		The parameter object.

	@return
		The parameter object for the first member of a uniform block or NULL if the parameter was
		malformed.

	@ingroup shacccg
*/
SceShaccCgParameter sceShaccCgGetFirstUniformBlockParameter(
	SceShaccCgParameter param);

/**	@brief	Returns the size of an array.

	Returns the size of an array.

	@param[in] param
		The parameter object.

	@return
		The size of an array parameter in terms of the number of elements.

	@ingroup shacccg
*/
uint32_t sceShaccCgGetArraySize(
	SceShaccCgParameter param);

/**	@brief	Returns the parameter for an array element.

	Returns the parameter for an array element.

	@param[in] aparam
		The array parameter object.

	@param[in] index
		The array index.

	@return
		The parameter object for the element associated with the array index.

	@ingroup shacccg
*/
SceShaccCgParameter sceShaccCgGetArrayParameter(
	SceShaccCgParameter aparam,
	uint32_t index);

/**	@brief	Returns the vector width for a vector parameter.

	Returns the vector width for a vector parameter.

	@param[in] param
		The vector parameter object.

	@return
		The width of the vector parameter.

	@ingroup shacccg
*/
uint32_t sceShaccCgGetParameterVectorWidth(
	SceShaccCgParameter param);

/**	@brief	Returns the number of columns for a matrix parameter.

	Returns the number of columns for a matrix parameter.

	@param[in] param
		The matrix parameter object.

	@return
		The number of columns for a matrix parameter

	@ingroup shacccg
*/
uint32_t sceShaccCgGetParameterColumns(
	SceShaccCgParameter param);

/**	@brief	Returns the number of rows for a matrix parameter.

	Returns the number of rows for a matrix parameter.

	@param[in] param
		The matrix parameter object.

	@return
		The number of rows for a matrix parameter

	@ingroup shacccg
*/
uint32_t sceShaccCgGetParameterRows(
	SceShaccCgParameter param);

/**	@brief	Returns the memory layout for a matrix parameter.

	Returns the memory layout for a matrix parameter.

	@param[in] param
		The matrix parameter object.

	@return
		The SceShaccCgParameterMemoryLayout for the matrix parameter.

	@ingroup shacccg
*/
SceShaccCgParameterMemoryLayout sceShaccCgGetParameterMemoryLayout(
	SceShaccCgParameter param);

/**	@brief	Returns the parameter for a row of a matrix parameter.

	Returns the parameter for a row of a matrix parameter.

	@param[in] param
		The matrix parameter object.

	@param[in] index
		The row index.

	@return
		The parameter object for the row paramater.

	@ingroup shacccg
*/
SceShaccCgParameter sceShaccCgGetRowParameter(
	SceShaccCgParameter param,
	uint32_t index);

/**	@brief	Returns the query format component count for a sampler parameter.

	Returns the query format component count for a sampler parameter.

	@param[in] param
		The sampler parameter object.

	@return
		The query format component count.

	@ingroup shacccg
*/
uint32_t sceShaccCgGetSamplerQueryFormatWidth(
	SceShaccCgParameter param);

/**	@brief	Returns the number of different precisions used to as query format for sampler parameter.

	Returns the number of different precisions used to as query format for sampler parameter.

	@param[in] param
		The sampler parameter object.

	@return
		count of different precisions used to as query format.

	@ingroup shacccg
*/
uint32_t sceShaccCgGetSamplerQueryFormatPrecisionCount(
	SceShaccCgParameter param);

/**	@brief	Returns query precision format for a sampler parameter.

	Returns query precision format for a sampler parameter.

	@param[in] param
		The sampler parameter object.

	@param[in] index
		The index of the precision format.

	@return
		query precision format.

	@ingroup shacccg
*/
SceShaccCgParameterBaseType sceShaccCgGetSamplerQueryFormatPrecision(
	SceShaccCgParameter param,
	uint32_t index);


#ifdef	__cplusplus
}
#endif	/* __cplusplus */

#endif /* _DOLCESDK_PSP2_SHACCCG_PARAMQUERY_H_ */
