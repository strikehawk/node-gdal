#ifndef __NODE_GDAL_VRTDATASET_H__
#define __NODE_GDAL_VRTDATASET_H__

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

// ogr
#include <ogrsf_frmts.h>

// vrt
#include <gdal_vrt.h>
#include <vrtdataset.h>

#include "../utils/obj_cache.hpp"

using namespace v8;
using namespace node;

namespace node_gdal {

class VrtDataset: public Nan::ObjectWrap {
public:
	static Nan::Persistent<FunctionTemplate> constructor;

	static void Initialize(Local<Object> target);
	static NAN_METHOD(New);
	static Local<Value> New(VRTDataset *ds);
	static NAN_METHOD(toString);
	// static NAN_METHOD(flush);
	// static NAN_METHOD(getMetadata);
	// static NAN_METHOD(getFileList);
	// static NAN_METHOD(getGCPProjection);
	// static NAN_METHOD(getGCPs);
	// static NAN_METHOD(setGCPs);
	// static NAN_METHOD(executeSQL);
	// static NAN_METHOD(testCapability);
	// static NAN_METHOD(buildOverviews);
	// static NAN_METHOD(close);

	// static NAN_GETTER(bandsGetter);
	// static NAN_GETTER(rasterSizeGetter);
	// static NAN_GETTER(srsGetter);
	// static NAN_GETTER(driverGetter);
	// static NAN_GETTER(geoTransformGetter);
	// static NAN_GETTER(descriptionGetter);
	// static NAN_GETTER(layersGetter);
	static NAN_GETTER(uidGetter);

	// static NAN_SETTER(srsSetter);
	// static NAN_SETTER(geoTransformSetter);

	static ObjectCache<VRTDataset, VrtDataset> vrtdataset_cache;

	VrtDataset(VRTDataset *ds);
	inline VRTDataset *getDataset() {
		return this_dataset;
	}

	void dispose();
	long uid;

	inline bool isAlive(){
		return this_dataset && ptr_manager.isAlive(uid);
	}

private:
	~VrtDataset();
	VRTDataset   *this_dataset;
};

}
#endif
