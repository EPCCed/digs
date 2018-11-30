// **********************************************************************
//
// Copyright (c) 2003-2008 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

// Ice version 3.3.0
// Generated from file `CommentAnnotation.ice'

#include <omero/model/CommentAnnotation.h>
#include <Ice/LocalException.h>
#include <Ice/ObjectFactory.h>
#include <Ice/BasicStream.h>
#include <IceUtil/Iterator.h>
#include <IceUtil/ScopedArray.h>

#ifndef ICE_IGNORE_VERSION
#   if ICE_INT_VERSION / 100 != 303
#       error Ice version mismatch!
#   endif
#   if ICE_INT_VERSION % 100 > 50
#       error Beta header file detected
#   endif
#   if ICE_INT_VERSION % 100 < 0
#       error Ice patch level mismatch!
#   endif
#endif

::Ice::Object* IceInternal::upCast(::omero::model::CommentAnnotation* p) { return p; }
::IceProxy::Ice::Object* IceInternal::upCast(::IceProxy::omero::model::CommentAnnotation* p) { return p; }

void
omero::model::__read(::IceInternal::BasicStream* __is, ::omero::model::CommentAnnotationPrx& v)
{
    ::Ice::ObjectPrx proxy;
    __is->read(proxy);
    if(!proxy)
    {
        v = 0;
    }
    else
    {
        v = new ::IceProxy::omero::model::CommentAnnotation;
        v->__copyFrom(proxy);
    }
}

void
omero::model::__writeCommentAnnotationAnnotationLinksSeq(::IceInternal::BasicStream* __os, const ::omero::model::AnnotationAnnotationLinkPtr* begin, const ::omero::model::AnnotationAnnotationLinkPtr* end)
{
    ::Ice::Int size = static_cast< ::Ice::Int>(end - begin);
    __os->writeSize(size);
    for(int i = 0; i < size; ++i)
    {
        __os->write(::Ice::ObjectPtr(::IceInternal::upCast(begin[i].get())));
    }
}

void
omero::model::__readCommentAnnotationAnnotationLinksSeq(::IceInternal::BasicStream* __is, ::omero::model::CommentAnnotationAnnotationLinksSeq& v)
{
    ::Ice::Int sz;
    __is->readSize(sz);
    __is->startSeq(sz, 4);
    v.resize(sz);
    for(int i = 0; i < sz; ++i)
    {
        __is->read(::omero::model::__patch__AnnotationAnnotationLinkPtr, &v[i]);
        __is->checkSeq();
        __is->endElement();
    }
    __is->endSeq(sz);
}

const ::std::string&
IceProxy::omero::model::CommentAnnotation::ice_staticId()
{
    return ::omero::model::CommentAnnotation::ice_staticId();
}

::IceInternal::Handle< ::IceDelegateM::Ice::Object>
IceProxy::omero::model::CommentAnnotation::__createDelegateM()
{
    return ::IceInternal::Handle< ::IceDelegateM::Ice::Object>(new ::IceDelegateM::omero::model::CommentAnnotation);
}

::IceInternal::Handle< ::IceDelegateD::Ice::Object>
IceProxy::omero::model::CommentAnnotation::__createDelegateD()
{
    return ::IceInternal::Handle< ::IceDelegateD::Ice::Object>(new ::IceDelegateD::omero::model::CommentAnnotation);
}

::IceProxy::Ice::Object*
IceProxy::omero::model::CommentAnnotation::__newInstance() const
{
    return new CommentAnnotation;
}

omero::model::CommentAnnotation::CommentAnnotation(const ::omero::RLongPtr& __ice_id, const ::omero::model::DetailsPtr& __ice_details, bool __ice_loaded, const ::omero::RStringPtr& __ice_ns, const ::omero::model::AnnotationAnnotationLinksSeq& __ice_annotationLinks, bool __ice_annotationLinksLoaded, const ::omero::sys::CountMap& __ice_annotationLinksCountPerOwner, const ::omero::RStringPtr& __ice_textValue) :
#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug
    IObject(__ice_id, __ice_details, __ice_loaded)
#else
    ::omero::model::IObject(__ice_id, __ice_details, __ice_loaded)
#endif
,
#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug
    Annotation(__ice_id, __ice_details, __ice_loaded, __ice_ns, __ice_annotationLinks, __ice_annotationLinksLoaded, __ice_annotationLinksCountPerOwner)
#else
    ::omero::model::Annotation(__ice_id, __ice_details, __ice_loaded, __ice_ns, __ice_annotationLinks, __ice_annotationLinksLoaded, __ice_annotationLinksCountPerOwner)
#endif
,
#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug
    TextAnnotation(__ice_id, __ice_details, __ice_loaded, __ice_ns, __ice_annotationLinks, __ice_annotationLinksLoaded, __ice_annotationLinksCountPerOwner, __ice_textValue)
#else
    ::omero::model::TextAnnotation(__ice_id, __ice_details, __ice_loaded, __ice_ns, __ice_annotationLinks, __ice_annotationLinksLoaded, __ice_annotationLinksCountPerOwner, __ice_textValue)
#endif

{
}

::Ice::ObjectPtr
omero::model::CommentAnnotation::ice_clone() const
{
    throw ::Ice::CloneNotImplementedException(__FILE__, __LINE__);
    return 0; // to avoid a warning with some compilers
}

static const ::std::string __omero__model__CommentAnnotation_ids[5] =
{
    "::Ice::Object",
    "::omero::model::Annotation",
    "::omero::model::CommentAnnotation",
    "::omero::model::IObject",
    "::omero::model::TextAnnotation"
};

bool
omero::model::CommentAnnotation::ice_isA(const ::std::string& _s, const ::Ice::Current&) const
{
    return ::std::binary_search(__omero__model__CommentAnnotation_ids, __omero__model__CommentAnnotation_ids + 5, _s);
}

::std::vector< ::std::string>
omero::model::CommentAnnotation::ice_ids(const ::Ice::Current&) const
{
    return ::std::vector< ::std::string>(&__omero__model__CommentAnnotation_ids[0], &__omero__model__CommentAnnotation_ids[5]);
}

const ::std::string&
omero::model::CommentAnnotation::ice_id(const ::Ice::Current&) const
{
    return __omero__model__CommentAnnotation_ids[2];
}

const ::std::string&
omero::model::CommentAnnotation::ice_staticId()
{
    return __omero__model__CommentAnnotation_ids[2];
}

void
omero::model::CommentAnnotation::__gcReachable(::IceInternal::GCCountMap& _c) const
{
#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug
    TextAnnotation::__gcReachable(_c);
#else
    ::omero::model::TextAnnotation::__gcReachable(_c);
#endif
}

void
omero::model::CommentAnnotation::__gcClear()
{
#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug
    TextAnnotation::__gcClear();
#else
    ::omero::model::TextAnnotation::__gcClear();
#endif
}

static ::std::string __omero__model__CommentAnnotation_all[] =
{
    "addAnnotationAnnotationLink",
    "ice_id",
    "ice_ids",
    "ice_isA",
    "ice_ping",
    "removeAnnotationAnnotationLink",
    "unload"
};

::Ice::DispatchStatus
omero::model::CommentAnnotation::__dispatch(::IceInternal::Incoming& in, const ::Ice::Current& current)
{
    ::std::pair< ::std::string*, ::std::string*> r = ::std::equal_range(__omero__model__CommentAnnotation_all, __omero__model__CommentAnnotation_all + 7, current.operation);
    if(r.first == r.second)
    {
        throw ::Ice::OperationNotExistException(__FILE__, __LINE__, current.id, current.facet, current.operation);
    }

    switch(r.first - __omero__model__CommentAnnotation_all)
    {
        case 0:
        {
            return ___addAnnotationAnnotationLink(in, current);
        }
        case 1:
        {
            return ___ice_id(in, current);
        }
        case 2:
        {
            return ___ice_ids(in, current);
        }
        case 3:
        {
            return ___ice_isA(in, current);
        }
        case 4:
        {
            return ___ice_ping(in, current);
        }
        case 5:
        {
            return ___removeAnnotationAnnotationLink(in, current);
        }
        case 6:
        {
            return ___unload(in, current);
        }
    }

    assert(false);
    throw ::Ice::OperationNotExistException(__FILE__, __LINE__, current.id, current.facet, current.operation);
}

void
omero::model::CommentAnnotation::__write(::IceInternal::BasicStream* __os) const
{
    __os->writeTypeId(ice_staticId());
    __os->startWriteSlice();
    __os->endWriteSlice();
#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug
    TextAnnotation::__write(__os);
#else
    ::omero::model::TextAnnotation::__write(__os);
#endif
}

void
omero::model::CommentAnnotation::__read(::IceInternal::BasicStream* __is, bool __rid)
{
    if(__rid)
    {
        ::std::string myId;
        __is->readTypeId(myId);
    }
    __is->startReadSlice();
    __is->endReadSlice();
#if defined(_MSC_VER) && (_MSC_VER < 1300) // VC++ 6 compiler bug
    TextAnnotation::__read(__is, true);
#else
    ::omero::model::TextAnnotation::__read(__is, true);
#endif
}

void
omero::model::CommentAnnotation::__write(const ::Ice::OutputStreamPtr&) const
{
    Ice::MarshalException ex(__FILE__, __LINE__);
    ex.reason = "type omero::model::CommentAnnotation was not generated with stream support";
    throw ex;
}

void
omero::model::CommentAnnotation::__read(const ::Ice::InputStreamPtr&, bool)
{
    Ice::MarshalException ex(__FILE__, __LINE__);
    ex.reason = "type omero::model::CommentAnnotation was not generated with stream support";
    throw ex;
}

void 
omero::model::__patch__CommentAnnotationPtr(void* __addr, ::Ice::ObjectPtr& v)
{
    ::omero::model::CommentAnnotationPtr* p = static_cast< ::omero::model::CommentAnnotationPtr*>(__addr);
    assert(p);
    *p = ::omero::model::CommentAnnotationPtr::dynamicCast(v);
    if(v && !*p)
    {
        IceInternal::Ex::throwUOE(::omero::model::CommentAnnotation::ice_staticId(), v->ice_id());
    }
}

bool
omero::model::operator==(const ::omero::model::CommentAnnotation& l, const ::omero::model::CommentAnnotation& r)
{
    return static_cast<const ::Ice::Object&>(l) == static_cast<const ::Ice::Object&>(r);
}

bool
omero::model::operator<(const ::omero::model::CommentAnnotation& l, const ::omero::model::CommentAnnotation& r)
{
    return static_cast<const ::Ice::Object&>(l) < static_cast<const ::Ice::Object&>(r);
}
