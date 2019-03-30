#ifndef __NODE_GDAL_VRTSOURCES_H__
#define __NODE_GDAL_VRTSOURCES_H__

// node
#include <node.h>
#include <node_object_wrap.h>

// nan
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <nan.h>
#pragma GCC diagnostic pop

// gdal
#include <gdal_priv.h>

#include "../../deps/libgdal/gdal/frmts/vrt/gdal_vrt.h"
#include "../../deps/libgdal/gdal/frmts/vrt/vrtdataset.h"

#include "../utils/obj_cache.hpp"

using namespace v8;
using namespace node;

namespace node_gdal {

class VrtSimpleSource: public Nan::ObjectWrap {
public:
    static Nan::Persistent<FunctionTemplate> constructor;
	static void Initialize(Local<Object> target);
	static NAN_METHOD(New);
    static Local<Value> New(VRTSimpleSource *src);

    static NAN_METHOD(toString);
    static NAN_METHOD(setSrcBand);
    static NAN_METHOD(setSrcMaskBand);
    static NAN_METHOD(setSrcWindow);
    static NAN_METHOD(setDstWindow);
    static NAN_METHOD(setNoDataValue);
    static NAN_METHOD(getMinimum);
    static NAN_METHOD(getMaximum);
    static NAN_METHOD(computeRasterMinMax);

    static NAN_GETTER(resamplingGetter);
    static NAN_GETTER(isSimpleSourceGetter);
    static NAN_GETTER(typeGetter);
    static NAN_GETTER(uidGetter);

    static NAN_SETTER(resamplingSetter);

    static ObjectCache<VRTSimpleSource, VrtSimpleSource> cache;

    VrtSimpleSource();
	VrtSimpleSource(VRTSimpleSource *src);
    inline bool isAlive(){
		return this_source;
	}
	inline VRTSimpleSource *get() {
		return this_source;
	}

	void dispose();
    long uid;
private:
    ~VrtSimpleSource();
    VRTSimpleSource *this_source;
};

}
#endif
