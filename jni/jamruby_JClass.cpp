#include "jamruby_JClass.h"
#include "safe_jni.hpp"
#include "jni_Log.h"
#include "jni_type_conversion.hpp"
extern "C" {
#include "mruby/class.h"
#include "mruby/data.h"
#include "mruby/string.h"
}
#include <cstdlib>
#include <cstring>
#include <cstddef>

#include <vector>

#include "jamruby_JObject.h"
#include "jamruby_JMethod.h"

#define SUPER_CLASS_NAME "JObject"
#define CLASS_NAME "JClass"

struct jclass_data {
	JNIEnv *env;
	jclass gref_jclass;
};

template <typename JType> class jni_functor {
private:
	JNIEnv *env_;
	jni_functor();
	jni_functor(jni_functor const&);
	jni_functor &operator=(jni_functor const&);
public:
	jni_functor(JNIEnv *env) : env_(env) {
	}
	~jni_functor() {
	}
	jvalue operator () (jni_type_t const &type, JType obj, jmethodID jmid, jvalue *args) const {
		JNIEnv *env = env_;
		jvalue ret;
		if (jni_type_is_array(type)) {
			ret.l = env->CallObjectMethodA(obj, jmid, args);
		} else {
			switch(jni_type_get_type(type)) {
			case JNI_TYPE_VOID:
				env->CallVoidMethodA(obj, jmid, args);
				break;
			case JNI_TYPE_BOOLEAN:
				ret.z = env->CallBooleanMethodA(obj, jmid, args);
				break;
			case JNI_TYPE_BYTE:
				ret.b = env->CallByteMethodA(obj, jmid, args);
				break;
			case JNI_TYPE_CHAR:
				ret.c = env->CallCharMethodA(obj, jmid, args);
				break;
			case JNI_TYPE_SHORT:
				ret.s = env->CallShortMethodA(obj, jmid, args);
				break;
			case JNI_TYPE_INT:
				ret.i = env->CallIntMethodA(obj, jmid, args);
				break;
			case JNI_TYPE_LONG:
				ret.j = env->CallLongMethodA(obj, jmid, args);
				break;
			case JNI_TYPE_FLOAT:
				ret.f = env->CallFloatMethodA(obj, jmid, args);
				break;
			case JNI_TYPE_DOUBLE:
				ret.d = env->CallDoubleMethodA(obj, jmid, args);
				break;
			case JNI_TYPE_OBJECT:
				ret.l = env->CallObjectMethodA(obj, jmid, args);
				break;
			default:
				// TODO handle error.
				break;
			}
		}
		return ret;
	}
};

template <> class jni_functor<jclass> {
private:
	JNIEnv *env_;
	jni_functor();
	jni_functor(jni_functor const&);
	jni_functor &operator=(jni_functor const&);
public:
	jni_functor(JNIEnv *env) : env_(env) {
	}
	~jni_functor() {
	}
	jvalue operator () (jni_type_t const &type, jclass cls, jmethodID jmid, jvalue *args) const {
		JNIEnv *env = env_;
		jvalue ret;
		if (jni_type_is_array(type)) {
			ret.l = env->CallStaticObjectMethodA(cls, jmid, args);
		} else {
			switch(jni_type_get_type(type)) {
			case JNI_TYPE_VOID:
				env->CallStaticVoidMethodA(cls, jmid, args);
				break;
			case JNI_TYPE_BOOLEAN:
				ret.z = env->CallStaticBooleanMethodA(cls, jmid, args);
				break;
			case JNI_TYPE_BYTE:
				ret.b = env->CallStaticByteMethodA(cls, jmid, args);
				break;
			case JNI_TYPE_CHAR:
				ret.c = env->CallStaticCharMethodA(cls, jmid, args);
				break;
			case JNI_TYPE_SHORT:
				ret.s = env->CallStaticShortMethodA(cls, jmid, args);
				break;
			case JNI_TYPE_INT:
				ret.i = env->CallStaticIntMethodA(cls, jmid, args);
				break;
			case JNI_TYPE_LONG:
				ret.j = env->CallStaticLongMethodA(cls, jmid, args);
				break;
			case JNI_TYPE_FLOAT:
				ret.f = env->CallStaticFloatMethodA(cls, jmid, args);
				break;
			case JNI_TYPE_DOUBLE:
				ret.d = env->CallStaticDoubleMethodA(cls, jmid, args);
				break;
			case JNI_TYPE_OBJECT:
				ret.l = env->CallStaticObjectMethodA(cls, jmid, args);
				break;
			default:
				// TODO handle error.
				break;
			}
		}
		return ret;
	}
};

static bool jcls_convert_mrb_value_to_jvalue(mrb_state *mrb, JNIEnv *env, mrb_value rval, jvalue &jval, jni_type_t type);
static mrb_value jcls_convert_jvalue_to_mrb_value(mrb_state *mrb, JNIEnv *env, jni_type_t const &type, jvalue const &ret);
template <typename JType> static jvalue call_method(mrb_state *mrb, JNIEnv *env, jni_type_t const &type, JType obj, jmethodID jmid, jvalue* args);

static void jcls_free(mrb_state *mrb, void *ptr)
{
	if (NULL != ptr) {
		jclass_data *p = static_cast<jclass_data*>(ptr);
		p->env->DeleteGlobalRef(p->gref_jclass);
		p->env = NULL;
		p->gref_jclass = NULL;
	}
	mrb_free(mrb, ptr);
}

static struct mrb_data_type jcls_type = { CLASS_NAME, jcls_free };

mrb_value jcls_make(mrb_state *mrb, JNIEnv *env, jclass cls)
{
	safe_jni::clear_exception ce(env);
	RClass *c = mrb_class_get(mrb, CLASS_NAME);
	if (NULL == c) {
		return mrb_nil_value();
	}
	jclass_data *ptr = static_cast<jclass_data*>(mrb_malloc(mrb, sizeof(jclass_data)));
	if (NULL == ptr) {
		return mrb_nil_value();
	}
	ptr->env = env;
	ptr->gref_jclass = static_cast<jclass>(env->NewGlobalRef(cls));
	return mrb_obj_value(Data_Wrap_Struct(mrb, c, &jcls_type, ptr));
}

mrb_value jcls_make(mrb_state *mrb, JNIEnv *env, char const * const name)
{
	safe_jni::clear_exception ce(env);
	safe_jni::safe_local_ref<jclass> jcls(env, env->FindClass(name));
	if (!jcls) {
		return mrb_nil_value();
	}
	return jcls_make(mrb, env, jcls.get());
}

mrb_value jcls_call_static(mrb_state *mrb, mrb_value self)
{
	mrb_value jmthd;
	int rb_argc;
	mrb_value *rb_argv;
	int const argc = mrb_get_args(mrb, "o*", &jmthd, &rb_argv, &rb_argc);
	if (argc < 1) {
		return mrb_nil_value();
	}
	if (!jmethod_is_jmethod(mrb, jmthd)) {
		return mrb_nil_value();
	}

	jclass_data *data = static_cast<jclass_data*>(mrb_get_datatype(mrb, self, &jcls_type));
	JNIEnv *env = data->env;

	jmethodID jmid = jmethod_get_jmethodID(mrb, jmthd);
	if (NULL == jmid) {
		// 'jmthd' is not 'JMethod' object.
		return mrb_nil_value();
	}

	std::vector<jvalue> jvals(rb_argc);
	if (static_cast<std::size_t>(rb_argc) > jvals.size()) {
		return mrb_nil_value();
	}

	std::vector<jni_type_t> types(rb_argc);
	if (static_cast<std::size_t>(rb_argc) > types.size()) {
		return mrb_nil_value();
	}

	if (!jmethod_get_argument_types(mrb, jmthd, &types[0], rb_argc)) {
		return mrb_nil_value();
	}

	for (int i = 0; i < rb_argc; ++i) {
		jcls_convert_mrb_value_to_jvalue(mrb, env, rb_argv[i], jvals[i], types[i]);
	}

	jni_type_t const type = jmethod_get_return_type(mrb, jmthd);
	jvalue const &ret = call_method(mrb, env, type, data->gref_jclass, jmid, &jvals[0]);
	return jcls_convert_jvalue_to_mrb_value(mrb, env, type, ret);
}

mrb_value jcls_call(mrb_state *mrb, mrb_value self)
{
	mrb_value jobj;
	mrb_value jmthd;
	int rb_argc;
	mrb_value *rb_argv;
	int const argc = mrb_get_args(mrb, "oo*", &jobj, &jmthd, &rb_argv, &rb_argc);
	if (argc < 2) {
		LOGE("%s: Too few arguments.", __func__);
		return mrb_nil_value();
	}
	if (!jobject_is_jobject(mrb, jobj)) {
		LOGE("%s: First argument is not 'JObject'.", __func__);
		return mrb_nil_value();
	}
	if (!jmethod_is_jmethod(mrb, jmthd)) {
		LOGE("%s: Second argument is not 'JMethod'.", __func__);
		return mrb_nil_value();
	}

	jclass_data *data = static_cast<jclass_data*>(mrb_get_datatype(mrb, self, &jcls_type));
	JNIEnv *env = data->env;

	jobject obj = jobject_get_jobject(mrb, jobj);
	if (NULL == obj) {
		LOGE("%s: 'JObject' is null.", __func__);
		return mrb_nil_value();
	}

	jmethodID jmid = jmethod_get_jmethodID(mrb, jmthd);
	if (NULL == jmid) {
		LOGE("%s: 'JMethod' is null.", __func__);
		return mrb_nil_value();
	}
	
	std::vector<jvalue> jvals(rb_argc);
	if (static_cast<std::size_t>(rb_argc) > jvals.size()) {
		LOGE("%s: Insufficient memory error.", __func__);
		return mrb_nil_value();
	}

	std::vector<jni_type_t> types(rb_argc);
	if (static_cast<std::size_t>(rb_argc) > types.size()) {
		LOGE("%s: Insufficient memory error.", __func__);
		return mrb_nil_value();
	}

	if (!jmethod_get_argument_types(mrb, jmthd, &types[0], rb_argc)) {
		LOGE("%s: Argument type mismatch.", __func__);
		return mrb_nil_value();
	}

	for (int i = 0; i < rb_argc; ++i) {
		jcls_convert_mrb_value_to_jvalue(mrb, env, rb_argv[i], jvals[i], types[i]);
	}

	jni_type_t const type = jmethod_get_return_type(mrb, jmthd);
	jvalue const &ret = call_method(mrb, env, type, obj, jmid, &jvals[0]); 
	return jcls_convert_jvalue_to_mrb_value(mrb, env, type, ret);
}

static mrb_value jcls_get_method(mrb_state *mrb, mrb_value self)
{
	mrb_value name, signature;
	int const argc = mrb_get_args(mrb, "SS", &name, &signature);
	if (2 != argc) {
		return mrb_nil_value();
	}
	jclass_data *data = static_cast<jclass_data*>(mrb_get_datatype(mrb, self, &jcls_type));
	JNIEnv *env = data->env;
	safe_jni::clear_exception ce(env);
	char const * const sig = mrb_string_value_ptr(mrb, signature);
	jmethodID jmid = env->GetMethodID(data->gref_jclass, mrb_string_value_ptr(mrb, name), sig);
	if (NULL == jmid) {
		return mrb_nil_value();
	}
	return jmethod_make(mrb, env, jmid, sig);
}

static mrb_value jcls_get_static_method(mrb_state *mrb, mrb_value self)
{
	mrb_value name, signature;
	int const argc = mrb_get_args(mrb, "SS", &name, &signature);
	if (2 != argc) {
		return mrb_nil_value();
	}
	jclass_data *data = static_cast<jclass_data*>(mrb_get_datatype(mrb, self, &jcls_type));
	JNIEnv *env = data->env;
	safe_jni::clear_exception ce(env);
	char const * const sig = mrb_string_value_ptr(mrb, signature);
	jmethodID jmid = env->GetStaticMethodID(data->gref_jclass, mrb_string_value_ptr(mrb, name), sig);
	if (NULL == jmid) {
		return mrb_nil_value();
	}
	return jmethod_make(mrb, env, jmid, sig);
}

static mrb_value jcls_get_class_object(mrb_state *mrb, mrb_value self)
{
	jclass_data *data = static_cast<jclass_data*>(mrb_get_datatype(mrb, self, &jcls_type));
	JNIEnv *env = data->env;
	safe_jni::safe_local_ref<jclass> objCls(env, env->GetObjectClass(data->gref_jclass));
	return jobject_make(mrb, env, objCls.get());
}

static mrb_value jcls_initialize(mrb_state *mrb, mrb_value self)
{
	return mrb_nil_value();
}

int jcls_init_class(mrb_state *mrb)
{
	RClass *cls_jobj = mrb_class_get(mrb, SUPER_CLASS_NAME);
	RClass *cls_jcls = mrb_define_class(mrb, CLASS_NAME, cls_jobj);
	if (NULL == cls_jcls) {
		return -1;
	}
	MRB_SET_INSTANCE_TT(cls_jcls, MRB_TT_DATA);
	mrb_define_method(mrb, cls_jcls, "get_method", jcls_get_method, ARGS_REQ(2));
	mrb_define_method(mrb, cls_jcls, "get_static_method", jcls_get_static_method, ARGS_REQ(2));
	mrb_define_method(mrb, cls_jcls, "get_class_object", jcls_get_class_object, ARGS_NONE());
	mrb_define_method(mrb, cls_jcls, "call_static", jcls_call_static, ARGS_REQ(1));
	mrb_define_method(mrb, cls_jcls, "call", jcls_call, ARGS_REQ(2));
	mrb_define_method(mrb, cls_jcls, "initialize", jcls_initialize, ARGS_REQ(1));
	return 0;
}

static bool jcls_convert_mrb_value_to_jvalue(mrb_state *mrb, JNIEnv *env, mrb_value rval, jvalue &jval, jni_type_t type)
{
	switch(mrb_type(rval)) {
	case MRB_TT_FALSE:
		if (jni_type_is_array(type)) {
			if (mrb_nil_p(rval)) {
				LOGD("MRB_TT_FALSE(NIL) -> JNI_TYPE_ARRAY");
				jval.l = NULL;
			} else {
				return false;
			}
		}
		switch(jni_type_get_type(type)) {
		case JNI_TYPE_BOOLEAN:
			LOGD("MRB_TT_FALSE -> JNI_TYPE_BOOLEAN");
			jval.z = JNI_FALSE;
			break;
		case JNI_TYPE_OBJECT:
			if (mrb_nil_p(rval)) {
				jval.l = NULL;
			} else {
				return false;
			}
			break;
		default:
			return false;
		}
		break;
	case MRB_TT_TRUE:
		switch(type) {
		case JNI_TYPE_BOOLEAN:
			LOGD("MRB_TT_TRUE -> JNI_TYPE_BOOLEAN");
			jval.z = JNI_TRUE;
			break;
		default:
			return false;
		}
		break;
	case MRB_TT_FIXNUM:
		switch(type) {
		case JNI_TYPE_BYTE:
			LOGD("MRB_TT_FIXNUM -> JNI_TYPE_BYTE");
			jval.b = static_cast<jbyte>(mrb_fixnum(rval));
			break;
		case JNI_TYPE_CHAR:
			LOGD("MRB_TT_FIXNUM -> JNI_TYPE_CHAR");
			jval.c = static_cast<jchar>(mrb_fixnum(rval));
			break;
		case JNI_TYPE_SHORT:
			LOGD("MRB_TT_FIXNUM -> JNI_TYPE_SHORT");
			jval.s = static_cast<jshort>(mrb_fixnum(rval));
			break;
		case JNI_TYPE_INT:
			LOGD("MRB_TT_FIXNUM -> JNI_TYPE_INT");
			jval.i = mrb_fixnum(rval);
			break;
		case JNI_TYPE_LONG:
			LOGD("MRB_TT_FIXNUM -> JNI_TYPE_LONG");
			jval.j = mrb_fixnum(rval);
			break;
		case JNI_TYPE_FLOAT:
			LOGD("MRB_TT_FIXNUM -> JNI_TYPE_FLOAT");
			jval.f = static_cast<jfloat>(mrb_fixnum(rval));
			break;
		case JNI_TYPE_DOUBLE:
			LOGD("MRB_TT_FIXNUM -> JNI_TYPE_DOUBLE");
			jval.d = static_cast<jdouble>(mrb_fixnum(rval));
			break;
		default:
			return false;
		}
		break;
	case MRB_TT_FLOAT:
		switch(type) {
		case JNI_TYPE_FLOAT:
			LOGD("MRB_TT_FLOAT -> JNI_TYPE_FLOAT");
			jval.f = static_cast<jfloat>(mrb_float(rval));
			break;
		case JNI_TYPE_DOUBLE:
			LOGD("MRB_TT_FLOAT -> JNI_TYPE_DOUBLE");
			jval.d = mrb_float(rval);
			break;
		default:
			return false;
		}
		break;
	case MRB_TT_STRING:
		switch(type) {
		case JNI_TYPE_OBJECT:
			{
				LOGD("MRB_TT_STRING -> JNI_TYPE_OBJECT");
				// TODO validate Java object type.
				jval.l = env->NewStringUTF(mrb_string_value_ptr(mrb, rval));
				break;
			}
		default:
			return false;
		}
		break;
	case MRB_TT_DATA:
		switch(type) {
		case JNI_TYPE_OBJECT:
			LOGD("MRB_TT_DATA -> JNI_TYPE_OBJECT");
			if (!jobject_is_jobject(mrb, rval)) {
				return false;
			}
			jval.l = jobject_get_jobject(mrb, rval);
			break;
		default:
			return false;
		}
		break;
	default:
		{
			LOGD("MRB_TT_XXX -> JNI_TYPE_OBJECT");
			jval.l = create_value(env, rval);
		}
		break;
	}
	return true;
}

static mrb_value jcls_convert_jvalue_to_mrb_value(mrb_state *mrb, JNIEnv *env, jni_type_t const &type, jvalue const &ret)
{
	if (jni_type_is_array(type)) {
		safe_jni::safe_local_ref<jobject> retval(env, ret.l);
		return (NULL == ret.l) ? mrb_nil_value() : jobject_make(mrb, env, retval.get());
	} else {
		switch(type) {
		case JNI_TYPE_VOID:
			return mrb_nil_value();
		case JNI_TYPE_BOOLEAN:
			return (ret.z != JNI_FALSE) ? mrb_true_value() : mrb_false_value();
		case JNI_TYPE_BYTE:
			return mrb_fixnum_value(ret.b);
		case JNI_TYPE_CHAR:
			return mrb_fixnum_value(ret.c);
		case JNI_TYPE_SHORT:
			return mrb_fixnum_value(ret.s);
		case JNI_TYPE_INT:
			return mrb_fixnum_value(ret.i);
		case JNI_TYPE_LONG:
			return mrb_fixnum_value((int32_t)ret.j); // TODO if 'Bignum' is supported by mruby, modify this code.
		case JNI_TYPE_FLOAT:
			return mrb_float_value(ret.f);
		case JNI_TYPE_DOUBLE:
#ifdef MRB_USE_FLOAT
			return mrb_float_value((float)ret.d);
#else
			return mrb_float_value(ret.d);
#endif
		case JNI_TYPE_OBJECT:
			{
				safe_jni::safe_local_ref<jobject> retval(env, ret.l);
				return (NULL == ret.l) ? mrb_nil_value() : jobject_make(mrb, env, retval.get());
			}
		default:
			return mrb_nil_value();
		}
	}
}

template <typename JType> static jvalue call_method(mrb_state *mrb, JNIEnv *env, jni_type_t const &type, JType obj, jmethodID jmid, jvalue* args)
{
	LOGD("enter JVM function.");
	jvalue const &ret = jni_functor<JType>(env)(type, obj, jmid, args);
	LOGD("leave JVM function.");
	safe_jni::safe_local_ref<jthrowable> e(env, env->ExceptionOccurred());
	if (NULL != e) {
		env->ExceptionClear();
		RClass *error_class = NULL;
		{
			safe_jni::safe_local_ref<jclass> jerror_class(env, env->FindClass("java/lang/Error"));
			if (JNI_FALSE != env->IsInstanceOf(e.get(), jerror_class.get())) {
				error_class = mrb_class_obj_get(mrb, "JError");
			}
		}
		if (NULL == error_class) {
			safe_jni::safe_local_ref<jclass> jexception_class(env, env->FindClass("java/lang/Exception"));
			if (JNI_FALSE != env->IsInstanceOf(e.get(), jexception_class.get())) {
				error_class = mrb_class_obj_get(mrb, "JException");
			}
		}
		if (NULL == error_class) {
			error_class = mrb_class_obj_get(mrb, "JThrowable");
		}
		env->ExceptionClear();
		char *msg = NULL;
		{
			safe_jni::safe_local_ref<jclass> cls(env, env->GetObjectClass(e.get()));
			if (cls.get()) {
				jmethodID mid = env->GetMethodID(cls.get(), "getMessage", "()Ljava/lang/String;");
				if (NULL != mid) {
					safe_jni::safe_local_ref<jstring> str(env, static_cast<jstring>(env->CallObjectMethod(e.get(), mid)));
					safe_jni::safe_string msg_str(env, str.get());
					msg = strdup(msg_str.string());
				}
			}
		}
		env->ExceptionClear();
		if (NULL == msg) {
			char const *err_msg = "Exception occurred inside JVM.";
			mrb_value const &exc = mrb_exc_new(mrb, error_class, err_msg, strlen(err_msg));
			mrb_exc_raise(mrb, exc);
		} else {
			mrb_value const &exc = mrb_exc_new(mrb, error_class, msg, strlen(msg));
			free(msg);
			msg = NULL;
			mrb_exc_raise(mrb, exc);
		}
	}
	return ret;
}