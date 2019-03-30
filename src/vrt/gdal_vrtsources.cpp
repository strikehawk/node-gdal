#include "../gdal_common.hpp"

#include "../gdal_rasterband.hpp"
#include "gdal_vrtsources.hpp"

#include <limits>
#include <cpl_port.h>

namespace node_gdal {

Nan::Persistent<FunctionTemplate> VrtSimpleSource::constructor;
ObjectCache<VRTSimpleSource, VrtSimpleSource> VrtSimpleSource::cache;

void VrtSimpleSource::Initialize(Local<Object> target)
{
    Nan::HandleScope scope;

	Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(VrtSimpleSource::New);
	lcons->InstanceTemplate()->SetInternalFieldCount(1);
	lcons->SetClassName(Nan::New("VrtSimpleSource").ToLocalChecked());

	Nan::SetPrototypeMethod(lcons, "toString", toString);
	Nan::SetPrototypeMethod(lcons, "setSrcBand", setSrcBand);
	Nan::SetPrototypeMethod(lcons, "setSrcMaskBand", setSrcMaskBand);
	Nan::SetPrototypeMethod(lcons, "setSrcWindow", setSrcWindow);
	Nan::SetPrototypeMethod(lcons, "setDstWindow", setDstWindow);
	Nan::SetPrototypeMethod(lcons, "getMinimum", getMinimum);
	Nan::SetPrototypeMethod(lcons, "getMaximum", getMaximum);
	Nan::SetPrototypeMethod(lcons, "computeRasterMinMax", computeRasterMinMax);

	ATTR_DONT_ENUM(lcons, "_uid", uidGetter, READ_ONLY_SETTER);
	ATTR(lcons, "resampling", resamplingGetter, resamplingSetter);
	ATTR(lcons, "isSimpleSource", isSimpleSourceGetter, READ_ONLY_SETTER);
	ATTR(lcons, "type", typeGetter, READ_ONLY_SETTER);

	target->Set(Nan::New("VrtSimpleSource").ToLocalChecked(), lcons->GetFunction());

	constructor.Reset(lcons);
}

VrtSimpleSource::VrtSimpleSource(VRTSimpleSource *src)
	: Nan::ObjectWrap(),
	  uid(0), 
	  this_source(src)
{
	LOG("Created VrtSimpleSource [%p]", src);
}

VrtSimpleSource::VrtSimpleSource()
	: Nan::ObjectWrap(), uid(0), this_source(0)
{
}

VrtSimpleSource::~VrtSimpleSource()
{
	//Destroy at garbage collection time if not already explicitly destroyed
	dispose();
}

void VrtSimpleSource::dispose()
{
    if (this_source) {
		LOG("Disposing VrtSimpleSource [%p]", this_source);

		LOG("Disposed VrtSimpleSource [%p]", this_source);

		this_source = NULL;
	}
}

NAN_METHOD(VrtSimpleSource::New)
{
	Nan::HandleScope scope;
	VrtSimpleSource *f;

	if (!info.IsConstructCall()) {
		Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
		return;
	}

	if (info[0]->IsExternal()) {
		Local<External> ext = info[0].As<External>();
		void* ptr = ext->Value();
		f = static_cast<VrtSimpleSource *>(ptr);

	} else {
		if (info.Length() == 0) {
			VRTSimpleSource *src = new VRTSimpleSource();
			f = new VrtSimpleSource(src);
		} else {
			VrtSimpleSource* poSrcSource;
			double dfXDstRatio = 0;
			double dfYDstRatio = 0;

			NODE_ARG_WRAPPED(0, "poSrcSource", VrtSimpleSource, poSrcSource);
			NODE_ARG_DOUBLE(1, "dfXDstRatio", dfXDstRatio);
			NODE_ARG_DOUBLE(2, "dfYDstRatio", dfYDstRatio);
			
			VRTSimpleSource *src = new VRTSimpleSource(poSrcSource->get(), dfXDstRatio, dfYDstRatio);
			f = new VrtSimpleSource(src);
		}
	}

	f->Wrap(info.This());
	info.GetReturnValue().Set(info.This());
}

Local<Value> VrtSimpleSource::New(VRTSimpleSource *raw)
{
	Nan::EscapableHandleScope scope;

	if (!raw) {
		return scope.Escape(Nan::Null());
	}
	if (cache.has(raw)) {
		return scope.Escape(cache.get(raw));
	}

	VrtSimpleSource *wrapped = new VrtSimpleSource(raw);

	Local<Value> ext = Nan::New<External>(wrapped);
	Local<Object> obj = Nan::NewInstance(Nan::New(VrtSimpleSource::constructor)->GetFunction(), 1, &ext).ToLocalChecked();

	cache.add(raw, obj);

	return scope.Escape(obj);
}

NAN_METHOD(VrtSimpleSource::toString)
{
	Nan::HandleScope scope;
	info.GetReturnValue().Set(Nan::New("VrtSimpleSource").ToLocalChecked());
}

/**
 * 
 * @throws Error
 * @method setSrcBand
 */
NAN_METHOD(VrtSimpleSource::setSrcBand)
{
	Nan::HandleScope scope;

    VrtSimpleSource *src = Nan::ObjectWrap::Unwrap<VrtSimpleSource>(info.This());
	if (!src->isAlive()) {
		Nan::ThrowError("VrtSimpleSource object has already been destroyed");
		return;
	}

	RasterBand* band;
    NODE_ARG_WRAPPED(0, "new source band", RasterBand, band);

	src->this_source->SetSrcBand(band->get());

	return;
}

/**
 * 
 * @throws Error
 * @method setSrcMaskBand
 * @param srcBand {gdal.RasterBand} Band is not the mask band, but the band from which the mask band is taken.
 */
NAN_METHOD(VrtSimpleSource::setSrcMaskBand)
{
	Nan::HandleScope scope;

    VrtSimpleSource *src = Nan::ObjectWrap::Unwrap<VrtSimpleSource>(info.This());
	if (!src->isAlive()) {
		Nan::ThrowError("VrtSimpleSource object has already been destroyed");
		return;
	}

	RasterBand* band;
    NODE_ARG_WRAPPED(0, "new source mask band", RasterBand, band);

	src->this_source->SetSrcMaskBand(band->get());

	return;
}

/**
 * 
 * @throws Error
 * @method setSrcWindow
 * @param xOff {Double}
 * @param yOff {Double}
 * @param xSize {Double}
 * @param ySize {Double}
 */
NAN_METHOD(VrtSimpleSource::setSrcWindow)
{
	Nan::HandleScope scope;

    VrtSimpleSource *src = Nan::ObjectWrap::Unwrap<VrtSimpleSource>(info.This());
	if (!src->isAlive()) {
		Nan::ThrowError("VrtSimpleSource object has already been destroyed");
		return;
	}

	double xOff;
	double yOff;
	double xSize;
	double ySize;
    NODE_ARG_DOUBLE(0, "x offset", xOff);
    NODE_ARG_DOUBLE(1, "y offset", yOff);
    NODE_ARG_DOUBLE(2, "x size", xSize);
    NODE_ARG_DOUBLE(3, "y size", ySize);

	src->this_source->SetSrcWindow(xOff, yOff, xSize, ySize);

	return;
}

/**
 * 
 * @throws Error
 * @method setDstWindow
 * @param xOff {Double}
 * @param yOff {Double}
 * @param xSize {Double}
 * @param ySize {Double}
 */
NAN_METHOD(VrtSimpleSource::setDstWindow)
{
	Nan::HandleScope scope;

    VrtSimpleSource *src = Nan::ObjectWrap::Unwrap<VrtSimpleSource>(info.This());
	if (!src->isAlive()) {
		Nan::ThrowError("VrtSimpleSource object has already been destroyed");
		return;
	}

	double xOff;
	double yOff;
	double xSize;
	double ySize;
    NODE_ARG_DOUBLE(0, "x offset", xOff);
    NODE_ARG_DOUBLE(1, "y offset", yOff);
    NODE_ARG_DOUBLE(2, "x size", xSize);
    NODE_ARG_DOUBLE(3, "y size", ySize);

	src->this_source->SetDstWindow(xOff, yOff, xSize, ySize);

	return;
}

/**
 * 
 * @throws Error
 * @method setNoDataValue
 * @param noDataValue {Double}
 */
NAN_METHOD(VrtSimpleSource::setNoDataValue)
{
	Nan::HandleScope scope;

    VrtSimpleSource *src = Nan::ObjectWrap::Unwrap<VrtSimpleSource>(info.This());
	if (!src->isAlive()) {
		Nan::ThrowError("VrtSimpleSource object has already been destroyed");
		return;
	}

	double noDataValue;
    NODE_ARG_DOUBLE(0, "no data value", noDataValue);

	src->this_source->SetNoDataValue(noDataValue);

	return;
}

/**
 * Get minimum value for this source.
 *
 * @throws Error
 * @method getMinimum
 * @param xSize {Integer}
 * @param ySize {Integer}
 * @return {Double}
 */
NAN_METHOD(VrtSimpleSource::getMinimum)
{
	Nan::HandleScope scope;
	VrtSimpleSource *src = Nan::ObjectWrap::Unwrap<VrtSimpleSource>(info.This());
	if (!src->isAlive()) {
		Nan::ThrowError("VrtSimpleSource object has already been destroyed");
		return;
	}

	int xSize;
	int ySize;
    NODE_ARG_INT(0, "x size", xSize);
    NODE_ARG_INT(1, "y size", ySize);

	int success = 0;
	double result = src->this_source->GetMinimum(xSize, ySize, &success);
	info.GetReturnValue().Set(Nan::New<Number>(result));
}

/**
 * Get maximum value for this source.
 *
 * @throws Error
 * @method getMaximum
 * @param xSize {Integer}
 * @param ySize {Integer}
 * @return {Double}
 */
NAN_METHOD(VrtSimpleSource::getMaximum)
{
	Nan::HandleScope scope;
	VrtSimpleSource *src = Nan::ObjectWrap::Unwrap<VrtSimpleSource>(info.This());
	if (!src->isAlive()) {
		Nan::ThrowError("VrtSimpleSource object has already been destroyed");
		return;
	}

	int xSize;
	int ySize;
    NODE_ARG_INT(0, "x size", xSize);
    NODE_ARG_INT(1, "y size", ySize);

	int success = 0;
	double result = src->this_source->GetMaximum(xSize, ySize, &success);
	info.GetReturnValue().Set(Nan::New<Number>(result));
}

// --- Custom error handling to handle VRT errors ---
// see: https://github.com/mapbox/mapnik-omnivore/issues/10

static std::string stats_file_err = "";
static CPLErrorHandler last_err_handler;
static void CPL_STDCALL statisticsErrorHandler(CPLErr eErrClass, int err_no, const char *msg) {
	if(err_no == CPLE_OpenFailed) {
		stats_file_err = msg;
	}
	if(last_err_handler) {
		last_err_handler(eErrClass, err_no, msg);
	}
}
static void pushStatsErrorHandler() {
	last_err_handler = CPLSetErrorHandler(statisticsErrorHandler);
}
static void popStatsErrorHandler() {
	if(!last_err_handler) return;
	CPLSetErrorHandler(last_err_handler);
}

/**
 * Computes raster minimum/maximum.
 *
 * Returns the minimum and maximum of all pixel values in this source. If approximate statistics are sufficient, the `allow_approximation`
 * argument can be set to `true` in which case overviews, or a subset of image tiles may be used in computing the statistics.
 *
 * @throws Error
 * @method computeRasterMinMax
 * @param {Boolean} allow_approximation If `true` statistics may be computed based on overviews or a subset of all tiles.
 * @return {Object} Statistics containing `"min"`, `"max"` properties.
 */
NAN_METHOD(VrtSimpleSource::computeRasterMinMax)
{
	Nan::HandleScope scope;
	VrtSimpleSource *src = Nan::ObjectWrap::Unwrap<VrtSimpleSource>(info.This());
	if (!src->isAlive()) {
		Nan::ThrowError("VrtSimpleSource object has already been destroyed");
		return;
	}

	int xSize;
	int ySize;
	int approx;
    NODE_ARG_INT(0, "x size", xSize);
    NODE_ARG_INT(1, "y size", ySize);
    NODE_ARG_BOOL(2, "allow approximation", approx);

	pushStatsErrorHandler();
	double minMax[2];
	CPLErr err = src->this_source->ComputeRasterMinMax(xSize, ySize, approx, minMax);
	popStatsErrorHandler();
	if (!stats_file_err.empty()){
		Nan::ThrowError(stats_file_err.c_str());
	} else if (err) {
		NODE_THROW_CPLERR(err);
		return;
	}

	Local<Object> result = Nan::New<Object>();
	result->Set(Nan::New("min").ToLocalChecked(), Nan::New<Number>(minMax[0]));
	result->Set(Nan::New("max").ToLocalChecked(), Nan::New<Number>(minMax[1]));

	info.GetReturnValue().Set(result);
}

/**
 * `"NEAREST"`, `"BILINEAR"`, `"CUBIC"`, `"CUBICSPLINE"`, `"LANCZOS"`, `"GAUSS"`, `"AVERAGE"`, `"MODE"`
 *
 * @attribute resampling
 * @type {String}
 */
NAN_GETTER(VrtSimpleSource::resamplingGetter)
{
	Nan::HandleScope scope;
	VrtSimpleSource *src = Nan::ObjectWrap::Unwrap<VrtSimpleSource>(info.This());
	if (!src->isAlive()) {
		Nan::ThrowError("VrtSimpleSource object has already been destroyed");
		return;
	}

	const char *result = src->this_source->GetResampling();
	info.GetReturnValue().Set(SafeString::New(result));
}

NAN_SETTER(VrtSimpleSource::resamplingSetter)
{
	Nan::HandleScope scope;
	VrtSimpleSource *src = Nan::ObjectWrap::Unwrap<VrtSimpleSource>(info.This());
	if (!src->isAlive()) {
		Nan::ThrowError("VrtSimpleSource object has already been destroyed");
		return;
	}

	if (!value->IsString()) {
		Nan::ThrowError("Unit type must be a string");
		return;
	}
	std::string input = *Nan::Utf8String(value);
	src->this_source->SetResampling(input.c_str());
}

NAN_GETTER(VrtSimpleSource::isSimpleSourceGetter)
{
	Nan::HandleScope scope;
	info.GetReturnValue().Set(Nan::True());
}

NAN_GETTER(VrtSimpleSource::typeGetter)
{
	Nan::HandleScope scope;
	info.GetReturnValue().Set(Nan::New("SimpleSource").ToLocalChecked());
}

NAN_GETTER(VrtSimpleSource::uidGetter)
{
	Nan::HandleScope scope;
	VrtSimpleSource *src = Nan::ObjectWrap::Unwrap<VrtSimpleSource>(info.This());
	info.GetReturnValue().Set(Nan::New((int)src->uid));
}

} // namespace node_gdal
