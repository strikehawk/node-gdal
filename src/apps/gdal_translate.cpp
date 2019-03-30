#include "../gdal_common.hpp"
#include "gdal_translate.hpp"
#include "../gdal_dataset.hpp"

#include <sstream>
#include <vector>
#include <string>
#include <gdal_translate_lib.cpp>

namespace node_gdal {
    namespace {
        // --- Custom error handling ---
        static CPLErrorHandler last_err_handler;
        static void CPL_STDCALL errorHandler(CPLErr eErrClass, int err_no, const char *msg) {
            if (last_err_handler) {
                last_err_handler(eErrClass, err_no, msg);
            }
            Nan::ThrowError(msg);
        }

        static void pushErrorHandler() {
            last_err_handler = CPLSetErrorHandler(errorHandler);
        }

        static void popErrorHandler() {
            if(!last_err_handler) return;
            CPLSetErrorHandler(last_err_handler);
        }

        // Utility methods
        char** toCharArray(std::vector<std::string> vector)
        {
            char** list = NULL;
            for(size_t i = 0; i < vector.size(); i++)
            {
                list = CSLAddString(list, vector[i].c_str());
            }
            
            return list;
        }

        std::vector<std::string> buildParametersVector(Local<Array> bbox, int width)
        {
            Local<Value> val;
            double minx;
            double miny;
            double maxx;
            double maxy;

            val = bbox->Get(0);
            if (!val->IsNumber()) {
                Nan::ThrowError("Bbox array must only contain numbers");
                return {};
            }
            minx = val->NumberValue();

            val = bbox->Get(1);
            if (!val->IsNumber()) {
                Nan::ThrowError("Bbox array must only contain numbers");
                return {};
            }
            miny = val->NumberValue();

            val = bbox->Get(2);
            if (!val->IsNumber()) {
                Nan::ThrowError("Bbox array must only contain numbers");
                return {};
            }
            maxx = val->NumberValue();

            val = bbox->Get(3);
            if (!val->IsNumber()) {
                Nan::ThrowError("Bbox array must only contain numbers");
                return {};
            }
            maxy = val->NumberValue();

            // double width = maxx - minx;
            // double height = maxy - miny;

            std::ostringstream ssMinx;
            ssMinx << minx;

            std::ostringstream ssMiny;
            ssMiny << miny;

            std::ostringstream ssMaxx;
            ssMaxx << maxx;

            std::ostringstream ssMaxy;
            ssMaxy << maxy;

            // std::ostringstream ssWidth;
            // ssWidth << width;
            
            // std::ostringstream ssHeight;
            // ssHeight << height;

            // std::ostringstream ssResolution;
            // ssResolution << resolution;


            std::vector<std::string> vector;
            vector.push_back("-of");
            vector.push_back("MEM");
            vector.push_back("-projwin");
            vector.push_back(ssMinx.str().c_str());
            vector.push_back(ssMaxy.str().c_str());
            vector.push_back(ssMaxx.str().c_str());
            vector.push_back(ssMiny.str().c_str());
            // vector.push_back("-tr");
            // vector.push_back(ssResolution.str().c_str());
            // vector.push_back(ssResolution.str().c_str());

            if (width != 0)
            {
                std::ostringstream ssWidth;
                ssWidth << width;
                vector.push_back("-outsize");
                vector.push_back(ssWidth.str().c_str());
                vector.push_back("0");
            }

            return vector;
        }

        void translateDataset(std::string srcFilePath, char** argsArray, Local<Array> array) 
        {
            // const char *resultName = "C:\\Projects\\GitHub\\result.tif";
            const char *resultName = "result";
            
            pushErrorHandler();

            // create source dataset from source file path
            GDALAccess access = GA_ReadOnly;
            GDALDatasetH hSrcDs = GDALOpenEx(srcFilePath.c_str(), GDAL_OF_RASTER | GDAL_OF_VERBOSE_ERROR, NULL, NULL, NULL);

            if (hSrcDs == NULL)
            {
                LOG("Source dataset is null");
                return;
            }

            // create translate options
            GDALTranslateOptions *options = GDALTranslateOptionsNew(argsArray, NULL);
            if (options == NULL)
            {
                LOG("Options are null");
                return;
            }

            GDALDatasetH hOutDs = GDALTranslate(resultName, hSrcDs, options, NULL);
            if (hOutDs == NULL)
            {
                LOG("Output dataset is null");
                return;
            }
            
            GDALRasterBandH hBand = GDALGetRasterBand(hOutDs, 1);
            if (hBand == NULL)
            {
                LOG("Raster band is null");
                return;
            }
            
            GDALRasterBand *poBand = static_cast<GDALRasterBand*>(hBand);
            GDALRWFlag flag = GF_Read;
            int width = poBand->GetXSize();
            int height = poBand->GetYSize();

            float* poBuffer = (float *) CPLMalloc(sizeof(float) * width * height);
            int size = width * height;

            CPLErr err = poBand->RasterIO(flag, 0, 0, width, height, poBuffer, width, height, GDT_Float32, 0, 0);

            if (err) {
                //NODE_THROW_CPLERR(err);
                LOG("Error reading");
                return;
            }

            // fill result array
            for (int i = 0; i < size; i++) {
                array->Set(i, Nan::New(poBuffer[i]));
            }

            // close buffer
            CPLFree(poBuffer);

            // close datasets
            GDALClose(hSrcDs);
            GDALClose(hOutDs);

            // free options
            GDALTranslateOptionsFree(options);

            popErrorHandler();
        }
    }

    void Translate::Initialize(Local<Object> target)
    {
        Nan::SetMethod(target, "translate", translate);
    }

    NAN_METHOD(Translate::translate)
    {
        Nan::HandleScope scope;

        std::string srcFilePath("");
        Local<Array> bbox;
        int width;

        NODE_ARG_STR(0, "srcFilePath", srcFilePath);
        NODE_ARG_ARRAY(1, "bbox", bbox);
        NODE_ARG_INT(2, "width", width);

        if (srcFilePath == "") {
            Nan::ThrowError("Source file path cannot be empty");
			return;
        }

        if (bbox->Length() != 4) {
            Nan::ThrowError("Bounding box should contain 4 elements");
			return;
        }

        if (width < 0) {
            Nan::ThrowError("Width must be positive");
			return; 
        }

        std::vector<std::string> vector = buildParametersVector(bbox, width);
        if (vector.empty()) return;

        char** argsArray = toCharArray(vector);

        Local<Array> array = Nan::New<Array>(0);
        translateDataset(srcFilePath, argsArray, array);

        info.GetReturnValue().Set(array);
    }
} // namespace node_gdal