#ifndef __GDAL_TRANSLATE_H__
#define __GDAL_TRANSLATE_H__

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

// utils
#include <gdal_utils.h>

using namespace v8;
using namespace node;

namespace node_gdal {
namespace Translate {

	void Initialize(Local<Object> target);

	NAN_METHOD(translate);
}
}

#endif