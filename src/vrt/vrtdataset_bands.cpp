#include "../gdal_common.hpp"
#include "gdal_vrtdataset.hpp"
#include "gdal_vrtsourcedrasterband.hpp"
#include "vrtdataset_bands.hpp"
#include "../utils/string_list.hpp"

namespace node_gdal {

Nan::Persistent<FunctionTemplate> VrtDatasetBands::constructor;

void VrtDatasetBands::Initialize(Local<Object> target)
{
	Nan::HandleScope scope;

	Local<FunctionTemplate> lcons = Nan::New<FunctionTemplate>(VrtDatasetBands::New);
	lcons->InstanceTemplate()->SetInternalFieldCount(1);
	lcons->SetClassName(Nan::New("VrtDatasetBands").ToLocalChecked());

	Nan::SetPrototypeMethod(lcons, "toString", toString);
	Nan::SetPrototypeMethod(lcons, "count", count);
	Nan::SetPrototypeMethod(lcons, "create", create);
	Nan::SetPrototypeMethod(lcons, "get", get);

	ATTR_DONT_ENUM(lcons, "ds", dsGetter, READ_ONLY_SETTER);

	target->Set(Nan::New("VrtDatasetBands").ToLocalChecked(), lcons->GetFunction());

	constructor.Reset(lcons);
}

VrtDatasetBands::VrtDatasetBands()
	: Nan::ObjectWrap()
{}

VrtDatasetBands::~VrtDatasetBands()
{}

/**
 * An encapsulation of a {{#crossLink "gdal.Dataset"}}Dataset{{/crossLink}}'s raster bands.
 *
 * ```
 * var bands = dataset.bands;```
 *
 * @class gdal.VrtDatasetBands
 */
NAN_METHOD(VrtDatasetBands::New)
{
	Nan::HandleScope scope;

	if (!info.IsConstructCall()) {
		Nan::ThrowError("Cannot call constructor as function, you need to use 'new' keyword");
		return;
	}
	if (info[0]->IsExternal()) {
		Local<External> ext = info[0].As<External>();
		void* ptr = ext->Value();
		VrtDatasetBands *f =  static_cast<VrtDatasetBands *>(ptr);
		f->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
		return;
	} else {
		Nan::ThrowError("Cannot create VrtDatasetBands directly");
		return;
	}
}

Local<Value> VrtDatasetBands::New(Local<Value> ds_obj)
{
	Nan::EscapableHandleScope scope;

	VrtDatasetBands *wrapped = new VrtDatasetBands();

	v8::Local<v8::Value> ext = Nan::New<External>(wrapped);
	v8::Local<v8::Object> obj = Nan::NewInstance(Nan::New(VrtDatasetBands::constructor)->GetFunction(), 1, &ext).ToLocalChecked();
	Nan::SetPrivate(obj, Nan::New("parent_").ToLocalChecked(), ds_obj);

	return scope.Escape(obj);
}

NAN_METHOD(VrtDatasetBands::toString)
{
	Nan::HandleScope scope;
	info.GetReturnValue().Set(Nan::New("VrtDatasetBands").ToLocalChecked());
}

/**
 * Returns the band with the given ID.
 *
 * @method get
 * @param {Integer} id
 * @return {gdal.RasterBand}
 */
NAN_METHOD(VrtDatasetBands::get)
{
	Nan::HandleScope scope;

	Local<Object> parent = Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(parent);

	if (!ds->isAlive()){
		Nan::ThrowError("VrtDataset object has already been destroyed");
		return;
	}

	
	VRTDataset* raw = ds->getDataset();
	int band_id;
	NODE_ARG_INT(0, "band id", band_id);

	VRTSourcedRasterBand *band = (VRTSourcedRasterBand*) raw->GetRasterBand(band_id);

	info.GetReturnValue().Set(VrtSourcedRasterBand::New(band, raw));
	return;
}

/**
 * Adds a new band.
 *
 * @method create
 * @throws Error
 * @param {Integer} dataType Type of band ({{#crossLink "Constants (GDT)"}}see GDT constants{{/crossLink}}).
 * @return {gdal.RasterBand}
 */
NAN_METHOD(VrtDatasetBands::create)
{
	Nan::HandleScope scope;

	Local<Object> parent = Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(parent);

	if (!ds->isAlive()) {
		Nan::ThrowError("VrtDataset object has already been destroyed");
		return;
	}

	VRTDataset* raw = ds->getDataset();
	GDALDataType type;
	StringList options;

	//NODE_ARG_ENUM(0, "data type", GDALDataType, type);
	if(info.Length() < 1) {
		Nan::ThrowError("data type argument needed");
		return;
	}
	if(info[0]->IsString()){
		std::string type_name = *Nan::Utf8String(info[0]);
		type = GDALGetDataTypeByName(type_name.c_str());
	} else if (info[0]->IsNull() || info[0]->IsUndefined()) {
		type = GDT_Unknown;
	} else {
		Nan::ThrowError("data type must be string or undefined");
		return;
	}

	if(info.Length() > 1 && options.parse(info[1])){
		return; //error parsing creation options
	}

	CPLErr err = raw->AddBand(type, options.get());

	if(err) {
		NODE_THROW_CPLERR(err);
		return;
	}

	VRTSourcedRasterBand *band = (VRTSourcedRasterBand*) raw->GetRasterBand(raw->GetRasterCount());

	info.GetReturnValue().Set(VrtSourcedRasterBand::New(band, raw));
}

/**
 * Returns the number of bands.
 *
 * @method count
 * @return {Integer}
 */
NAN_METHOD(VrtDatasetBands::count)
{
	Nan::HandleScope scope;

	Local<Object> parent = Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked().As<Object>();
	VrtDataset *ds = Nan::ObjectWrap::Unwrap<VrtDataset>(parent);

	if (!ds->isAlive()) {
		Nan::ThrowError("VrtDataset object has already been destroyed");
		return;
	}

	VRTDataset* raw = ds->getDataset();
	info.GetReturnValue().Set(Nan::New<Integer>(raw->GetRasterCount()));
}

/**
 * Parent dataset
 *
 * @readOnly
 * @attribute ds
 * @type {gdal.Dataset}
 */
NAN_GETTER(VrtDatasetBands::dsGetter)
{
	Nan::HandleScope scope;
	info.GetReturnValue().Set(Nan::GetPrivate(info.This(), Nan::New("parent_").ToLocalChecked()).ToLocalChecked());
}

} // namespace node_gdal
