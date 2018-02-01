
// fetchUnityVersion.cpp : Defines the entry point for the console application.
//

#include <nan.h>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace v8;
using namespace node;

#define THROW_BAD_ARGS Nan::ThrowTypeError("Bad argument")

struct store_data_t {
	Nan::Callback *cb;
	int arg;
	char *path;
	int error;
	char *unityVersion;
	int result;
};

#ifdef _WIN32


// returns NULL if the value could not be obtained
char *GetUnityVersion(std::string szFilename)
{
	// allocate a block of memory for the version info
	DWORD dummy;
	DWORD dwSize = GetFileVersionInfoSize(szFilename.c_str(), &dummy);
	if (dwSize == 0)
	{
		return NULL;
	}
	BYTE *data = new BYTE[dwSize];

	// load the version info
	if (!GetFileVersionInfo(szFilename.c_str(), NULL, dwSize, &data[0]))
	{
		delete[] data;
		return NULL;
	}

	wchar_t *pvUnityVersion = NULL;
	unsigned int iUnityVersionLen = 0;

	if (!VerQueryValue(&data[0], "\\StringFileInfo\\040904b0\\Unity Version", (LPVOID *)&pvUnityVersion, &iUnityVersionLen))
	{
		delete[] data;
		return NULL;
	}

	char *ret = new char[iUnityVersionLen];
	memcpy(ret, pvUnityVersion, iUnityVersionLen * sizeof(char));
	delete[] data;
	return ret;
}
#endif

static void ReqExecuted(uv_work_t *req) {
	Nan::HandleScope scope;

	store_data_t *store_data = static_cast<store_data_t *>(req->data);

	// there is always at least one argument. "error"
	int argc = 1;

	// Allocate space for two args: error plus possible additional result
	Local<Value> argv[2];
	Local<Object> statvfs_result;
	// NOTE: This may need to be changed if something returns a -1
	// for a success, which is possible.
	if (store_data->result == -1) {
		// If the request doesn't have a path parameter set.
		argv[0] = Nan::ErrnoException(store_data->error, "Execute_After", "No path specified");
	}
	else {
		// error value is empty or null for non-error.
		argv[0] = Nan::Null();

		argc = 2;
		if (store_data->unityVersion != NULL)
		{
		    argv[1] = Nan::New<String>(store_data->unityVersion).ToLocalChecked();
		}
		else
		{
		    argv[1] = Nan::New<String>("").ToLocalChecked();
		}
	}

	Nan::TryCatch try_catch;

	store_data->cb->Call(argc, argv);

	if (try_catch.HasCaught()) {
		Nan::FatalException(try_catch);
	}

	// Dispose of the persistent handle
	delete store_data->cb;
	delete store_data;
	delete req;
}

static void ExecuteReq(uv_work_t *req) {
	store_data_t* statvfs_data = static_cast<store_data_t *>(req->data);
	statvfs_data->result = 0;
#ifdef _WIN32
	statvfs_data->unityVersion = GetUnityVersion(statvfs_data->path);
#endif
	free(statvfs_data->path);
}

static NAN_METHOD(GetUnityVersion) {
  if (info.Length() < 1 ||
      !info[0]->IsString() ) {
    return THROW_BAD_ARGS;
  }

  String::Utf8Value path(info[0]->ToString());

  // Synchronous call needs much less work
  if (!info[1]->IsFunction()) {
#ifdef _WIN32
    char *unityVersion = GetUnityVersion(std::string(*path));
    if (unityVersion == NULL)
    {
        unityVersion = "";
    }
	Local<String> retval = Nan::New<String>(unityVersion).ToLocalChecked();
	info.GetReturnValue().Set(retval);
#else
    info.GetReturnValue().SetUndefined();
#endif
    return;
  }

  store_data_t *storeData = new store_data_t();
  storeData->cb = new Nan::Callback((Local<Function>) info[1].As<Function>());
  storeData->path = strdup(*path);
  uv_work_t *req = new uv_work_t;
  req->data = storeData;
  uv_queue_work(uv_default_loop(), req, ExecuteReq, (uv_after_work_cb)ReqExecuted);

  info.GetReturnValue().SetUndefined();
}

extern "C"
NAN_MODULE_INIT(init)
{
#ifdef _WIN32
	target->Set(Nan::New<String>("GetUnityVersion").ToLocalChecked(), Nan::New<FunctionTemplate>(GetUnityVersion)->GetFunction());
#endif
}


NODE_MODULE(editorVersion, init);