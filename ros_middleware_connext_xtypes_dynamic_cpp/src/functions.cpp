#include <iostream>
#include <stdexcept>

#include "ndds/ndds_cpp.h"

#include "rosidl_generator_cpp/MessageTypeSupport.h"
#include "ros_middleware_interface/handles.h"
#include "rosidl_typesupport_introspection_cpp/FieldTypes.h"
#include "rosidl_typesupport_introspection_cpp/MessageIntrospection.h"

namespace ros_middleware_interface
{

const char * _rti_connext_dynamic_identifier = "connext_dynamic";

ros_middleware_interface::NodeHandle create_node()
{
    std::cout << "create_node()" << std::endl;

    std::cout << "  create_node() get_instance" << std::endl;
    DDSDomainParticipantFactory* dpf_ = DDSDomainParticipantFactory::get_instance();
    if (!dpf_) {
        printf("  create_node() could not get participant factory\n");
        throw std::runtime_error("could not get participant factory");
    };

    DDS_DomainId_t domain = 0;

    std::cout << "  create_node() create_participant in domain " << domain << std::endl;
    DDSDomainParticipant* participant = dpf_->create_participant(
        domain, DDS_PARTICIPANT_QOS_DEFAULT, NULL,
        DDS_STATUS_MASK_NONE);
    if (!participant) {
        printf("  create_node() could not create participant\n");
        throw std::runtime_error("could not create participant");
    };

    std::cout << "  create_node() pass opaque node handle" << std::endl;

    ros_middleware_interface::NodeHandle node_handle = {
        _rti_connext_dynamic_identifier,
        participant
    };
    return node_handle;
}

DDS_TypeCode * create_type_code(std::string type_name, rosidl_typesupport_introspection_cpp::MessageMembers * members, DDS_DomainParticipantQos& participant_qos)
{
    DDS_TypeCodeFactory * factory = NULL;
    DDS_ExceptionCode_t ex = DDS_NO_EXCEPTION_CODE;
    DDS_StructMemberSeq struct_members;
    factory = DDS_TypeCodeFactory::get_instance();
    if (!factory) {
        printf("  create_type_code() could not get typecode factory\n");
        throw std::runtime_error("could not get typecode factory");
    };

    // TODO check how to remove the default value
    DDS_UnsignedLong max_string_size = 1024;
    DDS_Property_t * property = DDSPropertyQosPolicyHelper::lookup_property(participant_qos.property, "dds.builtin_type.string.max_size");
    if (property) {
        std::cout << "  lookup_property() " << property->name << " = " << property->value << std::endl;
        max_string_size = atoll(property->value);
    }

    DDS_TypeCode * type_code = factory->create_struct_tc(type_name.c_str(), struct_members, ex);
    for(unsigned long i = 0; i < members->member_count_; ++i)
    {
        const rosidl_typesupport_introspection_cpp::MessageMember * member = members->members_ + i;
        std::cout << "  create_type_code() create type code - add member " << i << ": " << member->name_ << std::endl;
        const DDS_TypeCode * member_type_code;
        switch (member->type_id_)
        {
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                member_type_code = factory->get_primitive_tc(DDS_TK_BOOLEAN);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                member_type_code = factory->get_primitive_tc(DDS_TK_OCTET);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                member_type_code = factory->get_primitive_tc(DDS_TK_CHAR);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                member_type_code = factory->get_primitive_tc(DDS_TK_FLOAT);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                member_type_code = factory->get_primitive_tc(DDS_TK_DOUBLE);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                member_type_code = factory->get_primitive_tc(DDS_TK_OCTET);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                member_type_code = factory->get_primitive_tc(DDS_TK_OCTET);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                member_type_code = factory->get_primitive_tc(DDS_TK_SHORT);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                member_type_code = factory->get_primitive_tc(DDS_TK_USHORT);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                member_type_code = factory->get_primitive_tc(DDS_TK_LONG);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                member_type_code = factory->get_primitive_tc(DDS_TK_ULONG);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                member_type_code = factory->get_primitive_tc(DDS_TK_LONGLONG);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                member_type_code = factory->get_primitive_tc(DDS_TK_ULONGLONG);
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                member_type_code = factory->create_string_tc(max_string_size, ex);
                break;
            default:
                printf("unknown type id %u\n", member->type_id_);
                throw std::runtime_error("unknown type");
        }
        type_code->add_member(member->name_, DDS_TYPECODE_MEMBER_ID_INVALID, member_type_code,
                    DDS_TYPECODE_NONKEY_REQUIRED_MEMBER, ex);
    }
    std::cout << "  type name: " << type_code->name(ex) << std::endl;
    DDS_StructMemberSeq_finalize(&struct_members);
    return type_code;
}


struct CustomPublisherInfo {
  DDSDynamicDataTypeSupport * dynamic_data_type_support_;
  DDSDynamicDataWriter * dynamic_writer_;
  DDS_TypeCode * type_code_;
  rosidl_typesupport_introspection_cpp::MessageMembers * members_;
  DDS_DynamicData * dynamic_data;
};

ros_middleware_interface::PublisherHandle create_publisher(const ros_middleware_interface::NodeHandle& node_handle, const rosidl_generator_cpp::MessageTypeSupportHandle & type_support_handle, const char * topic_name)
{
    std::cout << "create_publisher()" << std::endl;

    if (node_handle._implementation_identifier != _rti_connext_dynamic_identifier)
    {
        printf("node handle not from this implementation\n");
        printf("but from: %s\n", node_handle._implementation_identifier);
        throw std::runtime_error("node handle not from this implementation");
    }

    std::cout << "create_publisher() " << node_handle._implementation_identifier << std::endl;

    std::cout << "  create_publisher() extract participant from opaque node handle" << std::endl;
    DDSDomainParticipant* participant = (DDSDomainParticipant*)node_handle._data;

    if (type_support_handle._typesupport_identifier != rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier)
    {
        printf("type support not from this implementation\n");
        printf("but from: %s\n", type_support_handle._typesupport_identifier);
        throw std::runtime_error("type support not from this implementation");
    }

    rosidl_typesupport_introspection_cpp::MessageMembers * members = (rosidl_typesupport_introspection_cpp::MessageMembers*)type_support_handle._data;
    std::string type_name = std::string(members->package_name_) + "/" + members->message_name_;

    DDS_DomainParticipantQos participant_qos;
    DDS_ReturnCode_t status = participant->get_qos(participant_qos);
    if (status != DDS_RETCODE_OK) {
        printf("get_qos() failed. Status = %d\n", status);
        throw std::runtime_error("get participant qos failed");
    };

    std::cout << "  create_publisher() create type code for " << type_name.c_str() << std::endl;
    DDS_TypeCode * type_code = create_type_code(type_name, members, participant_qos);


    std::cout << "  create_publisher() register type code" << std::endl;
    DDSDynamicDataTypeSupport* ddts = new DDSDynamicDataTypeSupport(
        type_code, DDS_DYNAMIC_DATA_TYPE_PROPERTY_DEFAULT);
    status = ddts->register_type(participant, type_name.c_str());
    if (status != DDS_RETCODE_OK) {
        printf("register_type() failed. Status = %d\n", status);
        throw std::runtime_error("register type failed");
    };


    DDS_PublisherQos publisher_qos;
    status = participant->get_default_publisher_qos(publisher_qos);
    if (status != DDS_RETCODE_OK) {
        printf("get_default_publisher_qos() failed. Status = %d\n", status);
        throw std::runtime_error("get default publisher qos failed");
    };

    std::cout << "  create_publisher() create dds publisher" << std::endl;
    DDSPublisher* dds_publisher = participant->create_publisher(
        publisher_qos, NULL, DDS_STATUS_MASK_NONE);
    if (!dds_publisher) {
        printf("  create_publisher() could not create publisher\n");
        throw std::runtime_error("could not create publisher");
    };


    DDS_TopicQos default_topic_qos;
    status = participant->get_default_topic_qos(default_topic_qos);
    if (status != DDS_RETCODE_OK) {
        printf("get_default_topic_qos() failed. Status = %d\n", status);
        throw std::runtime_error("get default topic qos failed");
    };

    std::cout << "  create_publisher() create topic" << std::endl;
    DDSTopic* topic = participant->create_topic(
        topic_name, type_name.c_str(), default_topic_qos, NULL,
        DDS_STATUS_MASK_NONE
    );
    if (!topic) {
        printf("  create_topic() could not create topic\n");
        throw std::runtime_error("could not create topic");
    };


    DDS_DataWriterQos default_datawriter_qos;
    status = participant->get_default_datawriter_qos(default_datawriter_qos);
    if (status != DDS_RETCODE_OK) {
        printf("get_default_datawriter_qos() failed. Status = %d\n", status);
        throw std::runtime_error("get default datawriter qos failed");
    };

    std::cout << "  create_publisher() create data writer" << std::endl;
    DDSDataWriter* topic_writer = dds_publisher->create_datawriter(
        topic, default_datawriter_qos,
        NULL, DDS_STATUS_MASK_NONE);

    DDSDynamicDataWriter* dynamic_writer = DDSDynamicDataWriter::narrow(topic_writer);
    if (!dynamic_writer) {
        printf("narrow() failed.\n");
        throw std::runtime_error("narrow datawriter failed");
    };

    DDS_DynamicData * dynamic_data = ddts->create_data();

    std::cout << "  create_publisher() build opaque publisher handle" << std::endl;
    CustomPublisherInfo* custom_publisher_info = new CustomPublisherInfo();
    custom_publisher_info->dynamic_data_type_support_ = ddts;
    custom_publisher_info->dynamic_writer_ = dynamic_writer;
    custom_publisher_info->type_code_ = type_code;
    custom_publisher_info->members_ = members;
    custom_publisher_info->dynamic_data = dynamic_data;

    ros_middleware_interface::PublisherHandle publisher_handle = {
        _rti_connext_dynamic_identifier,
        custom_publisher_info
    };
    return publisher_handle;
}

//std::cout << "  publish() set member " << i << ": " << member->name_ << " = " << value << std::endl;
#define SET_VALUE(TYPE, METHOD_NAME) \
    { \
        TYPE value = *((TYPE*)((char*)ros_message + member->offset_)); \
        DDS_ReturnCode_t status = dynamic_data->METHOD_NAME(member->name_, DDS_DYNAMIC_DATA_MEMBER_ID_UNSPECIFIED, value); \
        if (status != DDS_RETCODE_OK) { \
            printf(#METHOD_NAME "() failed. Status = %d\n", status); \
            throw std::runtime_error("set member failed"); \
        } \
    }

#define SET_VALUE_WITH_SUFFIX(TYPE, METHOD_NAME, SUFFIX) \
    { \
        TYPE value = *((TYPE*)((char*)ros_message + member->offset_)); \
        DDS_ReturnCode_t status = dynamic_data->METHOD_NAME(member->name_, DDS_DYNAMIC_DATA_MEMBER_ID_UNSPECIFIED, value SUFFIX); \
        if (status != DDS_RETCODE_OK) { \
            printf(#METHOD_NAME "() failed. Status = %d\n", status); \
            throw std::runtime_error("set member failed"); \
        } \
    }

void publish(const ros_middleware_interface::PublisherHandle& publisher_handle, const void * ros_message)
{
    //std::cout << "publish()" << std::endl;


    if (publisher_handle._implementation_identifier != _rti_connext_dynamic_identifier)
    {
        printf("publisher handle not from this implementation\n");
        printf("but from: %s\n", publisher_handle._implementation_identifier);
        throw std::runtime_error("publisher handle not from this implementation");
    }

    //std::cout << "  publish() extract data writer and type code from opaque publisher handle" << std::endl;
    CustomPublisherInfo * custom_publisher_info = (CustomPublisherInfo*)publisher_handle._data;
    DDSDynamicDataTypeSupport* ddts = custom_publisher_info->dynamic_data_type_support_;
    DDSDynamicDataWriter * dynamic_writer = custom_publisher_info->dynamic_writer_;
    DDS_TypeCode * type_code = custom_publisher_info->type_code_;
    const rosidl_typesupport_introspection_cpp::MessageMembers * members = custom_publisher_info->members_;
    DDS_DynamicData * dynamic_data = custom_publisher_info->dynamic_data;

    //std::cout << "  publish() create " << members->package_name_ << "/" << members->message_name_ << " and populate dynamic data" << std::endl;
    //DDS_DynamicData dynamic_data(type_code, DDS_DYNAMIC_DATA_PROPERTY_DEFAULT);
    //DDS_DynamicData * dynamic_data = ddts->create_data();
    for(unsigned long i = 0; i < members->member_count_; ++i)
    {
        const rosidl_typesupport_introspection_cpp::MessageMember * member = members->members_ + i;
        //std::cout << "  publish() set member " << i << ": " << member->_name << std::endl;
        DDS_TypeCode * member_type_code;
        switch (member->type_id_)
        {
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                SET_VALUE(bool, set_boolean)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                SET_VALUE(uint8_t, set_octet)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                SET_VALUE(char, set_char)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                SET_VALUE(float, set_float)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                SET_VALUE(double, set_double)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                SET_VALUE(int8_t, set_octet)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                SET_VALUE(uint8_t, set_octet)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                SET_VALUE(int16_t, set_short)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                SET_VALUE(uint16_t, set_ushort)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                SET_VALUE(int32_t, set_long)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                SET_VALUE(uint32_t, set_ulong)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                SET_VALUE(int64_t, set_longlong)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                SET_VALUE(uint64_t, set_ulonglong)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                SET_VALUE_WITH_SUFFIX(std::string, set_string, .c_str())
                break;
            default:
                printf("unknown type id %u\n", member->type_id_);
                throw std::runtime_error("unknown type");
        }
    }

    //std::cout << "  publish() write dynamic data" << std::endl;
    DDS_ReturnCode_t status = dynamic_writer->write(*dynamic_data, DDS_HANDLE_NIL);
    if (status != DDS_RETCODE_OK) {
        printf("write() failed. Status = %d\n", status);
        throw std::runtime_error("write failed");
    };

    //ddts->delete_data(dynamic_data);
}

struct CustomSubscriberInfo {
  DDSDynamicDataTypeSupport * dynamic_data_type_support_;
  DDSDynamicDataReader * dynamic_reader_;
  DDS_TypeCode * type_code_;
  rosidl_typesupport_introspection_cpp::MessageMembers * members_;
  DDS_DynamicData * dynamic_data;
};

ros_middleware_interface::SubscriberHandle create_subscriber(const ros_middleware_interface::NodeHandle& node_handle, const rosidl_generator_cpp::MessageTypeSupportHandle & type_support_handle, const char * topic_name)
{
    std::cout << "create_subscriber()" << std::endl;

    if (node_handle._implementation_identifier != _rti_connext_dynamic_identifier)
    {
        printf("node handle not from this implementation\n");
        printf("but from: %s\n", node_handle._implementation_identifier);
        throw std::runtime_error("node handle not from this implementation");
    }

    std::cout << "create_subscriber() " << node_handle._implementation_identifier << std::endl;

    std::cout << "  create_subscriber() extract participant from opaque node handle" << std::endl;
    DDSDomainParticipant* participant = (DDSDomainParticipant*)node_handle._data;

    if (type_support_handle._typesupport_identifier != rosidl_typesupport_introspection_cpp::typesupport_introspection_identifier)
    {
        printf("type support not from this implementation\n");
        printf("but from: %s\n", type_support_handle._typesupport_identifier);
        throw std::runtime_error("type support not from this implementation");
    }

    rosidl_typesupport_introspection_cpp::MessageMembers * members = (rosidl_typesupport_introspection_cpp::MessageMembers*)type_support_handle._data;
    std::string type_name = std::string(members->package_name_) + "/" + members->message_name_;

    DDS_DomainParticipantQos participant_qos;
    DDS_ReturnCode_t status = participant->get_qos(participant_qos);
    if (status != DDS_RETCODE_OK) {
        printf("get_qos() failed. Status = %d\n", status);
        throw std::runtime_error("get participant qos failed");
    };

    std::cout << "  create_subscriber() create type code for " << type_name.c_str() << std::endl;
    DDS_TypeCode * type_code = create_type_code(type_name, members, participant_qos);


    std::cout << "  create_subscriber() register type code" << std::endl;
    DDSDynamicDataTypeSupport* ddts = new DDSDynamicDataTypeSupport(
        type_code, DDS_DYNAMIC_DATA_TYPE_PROPERTY_DEFAULT);
    status = ddts->register_type(participant, type_name.c_str());
    if (status != DDS_RETCODE_OK) {
        printf("register_type() failed. Status = %d\n", status);
        throw std::runtime_error("register type failed");
    };


    DDS_SubscriberQos subscriber_qos;
    status = participant->get_default_subscriber_qos(subscriber_qos);
    if (status != DDS_RETCODE_OK) {
        printf("get_default_subscriber_qos() failed. Status = %d\n", status);
        throw std::runtime_error("get default subscriber qos failed");
    };

    std::cout << "  create_subscriber() create dds subscriber" << std::endl;
    DDSSubscriber* dds_subscriber = participant->create_subscriber(
        subscriber_qos, NULL, DDS_STATUS_MASK_NONE);
    if (!dds_subscriber) {
        printf("  create_subscriber() could not create subscriber\n");
        throw std::runtime_error("could not create subscriber");
    };


    DDS_TopicQos default_topic_qos;
    status = participant->get_default_topic_qos(default_topic_qos);
    if (status != DDS_RETCODE_OK) {
        printf("get_default_topic_qos() failed. Status = %d\n", status);
        throw std::runtime_error("get default topic qos failed");
    };

    std::cout << "  create_subscriber() create topic" << std::endl;
    DDSTopic* topic = participant->create_topic(
        topic_name, type_name.c_str(), default_topic_qos, NULL,
        DDS_STATUS_MASK_NONE
    );
    if (!topic) {
        printf("  create_topic() could not create topic\n");
        throw std::runtime_error("could not create topic");
    };


    DDS_DataReaderQos default_datareader_qos;
    status = participant->get_default_datareader_qos(default_datareader_qos);
    if (status != DDS_RETCODE_OK) {
        printf("get_default_datareader_qos() failed. Status = %d\n", status);
        throw std::runtime_error("get default datareader qos failed");
    };

    std::cout << "  create_subscriber() create data reader" << std::endl;
    DDSDataReader* topic_reader = dds_subscriber->create_datareader(
        topic, default_datareader_qos,
        NULL, DDS_STATUS_MASK_NONE);

    DDSDynamicDataReader* dynamic_reader = DDSDynamicDataReader::narrow(topic_reader);
    if (!dynamic_reader) {
        printf("narrow() failed.\n");
        throw std::runtime_error("narrow datareader failed");
    };

    DDS_DynamicData * dynamic_data = ddts->create_data();

    std::cout << "  create_subscriber() build opaque subscriber handle" << std::endl;
    CustomSubscriberInfo* custom_subscriber_info = new CustomSubscriberInfo();
    custom_subscriber_info->dynamic_data_type_support_ = ddts;
    custom_subscriber_info->dynamic_reader_ = dynamic_reader;
    custom_subscriber_info->type_code_ = type_code;
    custom_subscriber_info->members_ = members;
    custom_subscriber_info->dynamic_data = dynamic_data;

    ros_middleware_interface::SubscriberHandle subscriber_handle = {
        _rti_connext_dynamic_identifier,
        custom_subscriber_info
    };
    return subscriber_handle;
}

//std::cout << "  take() get member " << i << ": " << member->name_ << " = " << value << std::endl;
#define GET_VALUE(TYPE, METHOD_NAME) \
    { \
        TYPE value; \
        DDS_ReturnCode_t status = dynamic_data->METHOD_NAME(value, member->name_, DDS_DYNAMIC_DATA_MEMBER_ID_UNSPECIFIED); \
        if (status != DDS_RETCODE_OK) { \
            printf(#METHOD_NAME "() failed. Status = %d\n", status); \
            throw std::runtime_error("get member failed"); \
        } \
        TYPE* ros_value = (TYPE*)((char*)ros_message + member->offset_); \
        *ros_value = value; \
    }

#define GET_VALUE_WITH_DIFFERENT_TYPES(TYPE, DDS_TYPE, METHOD_NAME) \
    { \
        DDS_TYPE value; \
        DDS_ReturnCode_t status = dynamic_data->METHOD_NAME(value, member->name_, DDS_DYNAMIC_DATA_MEMBER_ID_UNSPECIFIED); \
        if (status != DDS_RETCODE_OK) { \
            printf(#METHOD_NAME "() failed. Status = %d\n", status); \
            throw std::runtime_error("get member failed"); \
        } \
        TYPE* ros_value = (TYPE*)((char*)ros_message + member->offset_); \
        *ros_value = value; \
    }

bool take(const ros_middleware_interface::SubscriberHandle& subscriber_handle, void * ros_message)
{
    //std::cout << "take()" << std::endl;


    if (subscriber_handle.implementation_identifier_ != _rti_connext_dynamic_identifier)
    {
        printf("subscriber handle not from this implementation\n");
        printf("but from: %s\n", subscriber_handle.implementation_identifier_);
        throw std::runtime_error("subscriber handle not from this implementation");
    }

    //std::cout << "  take() extract data writer and type code from opaque subscriber handle" << std::endl;
    CustomSubscriberInfo * custom_subscriber_info = (CustomSubscriberInfo*)subscriber_handle.data_;
    DDSDynamicDataTypeSupport* ddts = custom_subscriber_info->dynamic_data_type_support_;
    DDSDynamicDataReader * dynamic_reader = custom_subscriber_info->dynamic_reader_;
    DDS_TypeCode * type_code = custom_subscriber_info->type_code_;
    const rosidl_typesupport_introspection_cpp::MessageMembers * members = custom_subscriber_info->members_;
    DDS_DynamicData * dynamic_data = custom_subscriber_info->dynamic_data;

    DDS_SampleInfo sample_info;
    // TODO use take / return_loan instead
    DDS_ReturnCode_t status = dynamic_reader->take_next_sample(*dynamic_data, sample_info);
    if (status == DDS_RETCODE_NO_DATA) {
        return false;
    }
    if (status != DDS_RETCODE_OK) {
        printf("take_next_sample() failed. Status = %d\n", status);
        throw std::runtime_error("take next sample failed");
    };

    if (ros_message == 0) {
        printf("take() invoked without a valid ROS message pointer\n");
        throw std::runtime_error("invalid ROS message pointer");
    };

    //std::cout << "  take() create " << members->package_name_ << "/" << members->message_name_ << " and populate dynamic data" << std::endl;
    //DDS_DynamicData dynamic_data(type_code, DDS_DYNAMIC_DATA_PROPERTY_DEFAULT);
    //DDS_DynamicData * dynamic_data = ddts->create_data();
    for(unsigned long i = 0; i < members->member_count_; ++i)
    {
        const rosidl_typesupport_introspection_cpp::MessageMember * member = members->members_ + i;
        //std::cout << "  take() get member " << i << ": " << member->_name << std::endl;
        switch (member->type_id_)
        {
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BOOL:
                GET_VALUE_WITH_DIFFERENT_TYPES(bool, DDS_Boolean, get_boolean)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_BYTE:
                GET_VALUE(uint8_t, get_octet)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_CHAR:
                GET_VALUE(char, get_char)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT32:
                GET_VALUE(float, get_float)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_FLOAT64:
                GET_VALUE(double, get_double)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT8:
                GET_VALUE_WITH_DIFFERENT_TYPES(int8_t, DDS_Octet,get_octet)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8:
                GET_VALUE(uint8_t, get_octet)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16:
                GET_VALUE(int16_t, get_short)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16:
                GET_VALUE(uint16_t, get_ushort)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT32:
                GET_VALUE(int32_t, get_long)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT32:
                GET_VALUE(uint32_t, get_ulong)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT64:
                GET_VALUE_WITH_DIFFERENT_TYPES(int64_t, DDS_LongLong, get_longlong)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT64:
                GET_VALUE_WITH_DIFFERENT_TYPES(uint64_t, DDS_UnsignedLongLong, get_ulonglong)
                break;
            case ::rosidl_typesupport_introspection_cpp::ROS_TYPE_STRING:
                {
                    char * value = 0;
                    DDS_UnsignedLong size;
                    DDS_ReturnCode_t status = dynamic_data->get_string(value, &size, member->name_, DDS_DYNAMIC_DATA_MEMBER_ID_UNSPECIFIED);
                    if (status != DDS_RETCODE_OK) {
                        printf("get_string() failed. Status = %d\n", status);
                        throw std::runtime_error("get member failed");
                    }
                    std::string* ros_value = (std::string*)((char*)ros_message + member->offset_);
                    *ros_value = value;
                    delete[] value;
                }
                break;
            default:
                printf("unknown type id %u\n", member->type_id_);
                throw std::runtime_error("unknown type");
        }
    }

    //ddts->delete_data(dynamic_data);

    return true;
}

ros_middleware_interface::GuardConditionHandle create_guard_condition()
{
    ros_middleware_interface::GuardConditionHandle guard_condition_handle;
    guard_condition_handle.implementation_identifier_ = _rti_connext_dynamic_identifier;
    guard_condition_handle.data_ = new DDSGuardCondition();
    return guard_condition_handle;
}

void trigger_guard_condition(const ros_middleware_interface::GuardConditionHandle& guard_condition_handle)
{
    //std::cout << "trigger_guard_condition()" << std::endl;

    if (guard_condition_handle.implementation_identifier_ != _rti_connext_dynamic_identifier)
    {
        printf("guard condition handle not from this implementation\n");
        printf("but from: %s\n", guard_condition_handle.implementation_identifier_);
        throw std::runtime_error("guard condition handle not from this implementation");
    }

    DDSGuardCondition * guard_condition = (DDSGuardCondition*)guard_condition_handle.data_;
    guard_condition->set_trigger_value(DDS_BOOLEAN_TRUE);
}

void wait(ros_middleware_interface::SubscriberHandles& subscriber_handles, ros_middleware_interface::GuardConditionHandles& guard_condition_handles)
{
    //std::cout << "wait()" << std::endl;

    DDSWaitSet waitset;

    // add a condition for each subscriber
    for (unsigned long i = 0; i < subscriber_handles.subscriber_count_; ++i)
    {
        void * data = subscriber_handles.subscribers_[i];
        CustomSubscriberInfo * custom_subscriber_info = (CustomSubscriberInfo*)data;
        DDSDynamicDataReader * dynamic_reader = custom_subscriber_info->dynamic_reader_;
        DDSStatusCondition * condition = dynamic_reader->get_statuscondition();
        condition->set_enabled_statuses(DDS_DATA_AVAILABLE_STATUS);
        waitset.attach_condition(condition);
    }

    // add a condition for each guard condition
    for (unsigned long i = 0; i < guard_condition_handles.guard_condition_count_; ++i)
    {
        void * data = guard_condition_handles.guard_conditions_[i];
        DDSGuardCondition * guard_condition = (DDSGuardCondition*)data;
        waitset.attach_condition(guard_condition);
    }

    // invoke wait until one of the conditions triggers
    DDSConditionSeq active_conditions;
    DDS_Duration_t timeout = DDS_Duration_t::from_seconds(1);
    DDS_ReturnCode_t status = DDS_RETCODE_TIMEOUT;
    while (DDS_RETCODE_TIMEOUT == status)
    {
        status = waitset.wait(active_conditions, timeout);
        if (DDS_RETCODE_TIMEOUT == status) {
            //std::cout << "wait() no data - rinse and repeat" << std::endl;
            continue;
        };
        if (status != DDS_RETCODE_OK) {
            printf("wait() failed. Status = %d\n", status);
            throw std::runtime_error("wait failed");
        };
    }

    // set subscriber handles to zero for all not triggered conditions
    for (unsigned long i = 0; i < subscriber_handles.subscriber_count_; ++i)
    {
        void * data = subscriber_handles.subscribers_[i];
        CustomSubscriberInfo * custom_subscriber_info = (CustomSubscriberInfo*)data;
        DDSDynamicDataReader * dynamic_reader = custom_subscriber_info->dynamic_reader_;
        DDSCondition * condition = dynamic_reader->get_statuscondition();

        // search for subscriber condition in active set
        unsigned long j = 0;
        for (; j < active_conditions.length(); ++j)
        {
            if (active_conditions[j] == condition)
            {
                break;
            }
        }
        // if subscriber condition is not found in the active set
        // reset the subscriber handle
        if (!(j < active_conditions.length()))
        {
            subscriber_handles.subscribers_[i] = 0;
        }
    }

    // set subscriber handles to zero for all not triggered conditions
    for (unsigned long i = 0; i < guard_condition_handles.guard_condition_count_; ++i)
    {
        void * data = guard_condition_handles.guard_conditions_[i];
        DDSCondition * condition = (DDSCondition*)data;

        // search for guard condition in active set
        unsigned long j = 0;
        for (; j < active_conditions.length(); ++j)
        {
            if (active_conditions[j] == condition)
            {
                break;
            }
        }
        // if guard condition is not found in the active set
        // reset the guard handle
        if (!(j < active_conditions.length()))
        {
            guard_condition_handles.guard_conditions_[i] = 0;
        }
    }
}

}