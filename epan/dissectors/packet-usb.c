/* packet-usb.c
 *
 * $Id$
 *
 * USB basic dissector
 * By Paolo Abeni <paolo.abeni@email.it>
 * Ronnie Sahlberg 2006
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib.h>
#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/etypes.h>
#include <epan/addr_resolv.h>
#include <epan/emem.h>
#include <epan/tap.h>
#include <epan/conversation.h>
#include <string.h>
#include "packet-usb.h"

/* protocols and header fields */
static int proto_usb = -1;
static int hf_usb_urb_id = -1;
static int hf_usb_bus_id = -1;
static int hf_usb_transfer_type = -1;
static int hf_usb_urb_type = -1;
static int hf_usb_device_address = -1;
static int hf_usb_data_flag = -1;
static int hf_usb_setup_flag = -1;
static int hf_usb_endpoint_number = -1;
static int hf_usb_src_endpoint_number = -1;
static int hf_usb_dst_endpoint_number = -1;
static int hf_usb_request = -1;
static int hf_usb_value = -1;
static int hf_usb_index = -1;
static int hf_usb_length = -1;
static int hf_usb_data = -1;
static int hf_usb_bmRequestType = -1;
static int hf_usb_bmRequestType_direction = -1;
static int hf_usb_bmRequestType_type = -1;
static int hf_usb_bmRequestType_recipient = -1;
static int hf_usb_bDescriptorType = -1;
static int hf_usb_descriptor_index = -1;
static int hf_usb_language_id = -1;
static int hf_usb_bLength = -1;
static int hf_usb_bcdUSB = -1;
static int hf_usb_bDeviceClass = -1;
static int hf_usb_bDeviceSubClass = -1;
static int hf_usb_bDeviceProtocol = -1;
static int hf_usb_bMaxPacketSize0 = -1;
static int hf_usb_idVendor = -1;
static int hf_usb_idProduct = -1;
static int hf_usb_bcdDevice = -1;
static int hf_usb_iManufacturer = -1;
static int hf_usb_iProduct = -1;
static int hf_usb_iSerialNumber = -1;
static int hf_usb_bNumConfigurations = -1;
static int hf_usb_wLANGID = -1;
static int hf_usb_bString = -1;
static int hf_usb_bInterfaceNumber = -1;
static int hf_usb_bAlternateSetting = -1;
static int hf_usb_bNumEndpoints = -1;
static int hf_usb_bInterfaceClass = -1;
static int hf_usb_bInterfaceSubClass = -1;
static int hf_usb_bInterfaceProtocol = -1;
static int hf_usb_iInterface = -1;
static int hf_usb_bEndpointAddress = -1;
static int hf_usb_bmAttributes = -1;
static int hf_usb_bEndpointAttributeTransfer = -1;
static int hf_usb_bEndpointAttributeSynchonisation = -1;
static int hf_usb_bEndpointAttributeBehaviour = -1;
static int hf_usb_wMaxPacketSize = -1;
static int hf_usb_bInterval = -1;
static int hf_usb_wTotalLength = -1;
static int hf_usb_bNumInterfaces = -1;
static int hf_usb_bConfigurationValue = -1;
static int hf_usb_iConfiguration = -1;
static int hf_usb_bMaxPower = -1;
static int hf_usb_configuration_bmAttributes = -1;
static int hf_usb_configuration_legacy10buspowered = -1;
static int hf_usb_configuration_selfpowered = -1;
static int hf_usb_configuration_remotewakeup = -1;
static int hf_usb_bEndpointAddress_direction = -1;
static int hf_usb_bEndpointAddress_number = -1;
static int hf_usb_response_in = -1;
static int hf_usb_time = -1;
static int hf_usb_request_in = -1;

static gint usb_hdr = -1;
static gint usb_setup_hdr = -1;
static gint ett_usb_setup_bmrequesttype = -1;
static gint ett_descriptor_device = -1;
static gint ett_configuration_bmAttributes = -1;
static gint ett_configuration_bEndpointAddress = -1;
static gint ett_endpoint_bmAttributes = -1;


static int usb_tap = -1;

static dissector_table_t usb_bulk_dissector_table;
static dissector_table_t usb_control_dissector_table;

static const value_string usb_langid_vals[] = {
    {0x0000,	"no language specified"},
    {0x0409,	"English (United States)"},
    {0, NULL}
};

static const value_string usb_interfaceclass_vals[] = {
    {IF_CLASS_FROM_INTERFACE_DESC,	"Use class info in Interface Descriptor"},
    {IF_CLASS_AUDIO,			"AUDIO"},
    {IF_CLASS_COMMUNICATIONS,		"COMMUNICATIONS"},
    {IF_CLASS_HID,			"HID"},
    {IF_CLASS_PHYSICAL,			"PHYSICAL"},
    {IF_CLASS_IMAGE,			"IMAGE"},
    {IF_CLASS_PRINTER,			"PRINTER"},
    {IF_CLASS_MASSTORAGE,		"MASSTORAGE"},
    {IF_CLASS_HUB,			"HUB"},
    {IF_CLASS_CDC_DATA,			"CDC_DATA"},
    {IF_CLASS_SMART_CARD,		"SMART_CARD"},
    {IF_CLASS_CONTENT_SECURITY,		"CONTENT_SECURITY"},
    {IF_CLASS_VIDEO,			"VIDEO"},
    {IF_CLASS_DIAGNOSTIC_DEVICE,	"DIAGNOSTIC_DEVICE"},
    {IF_CLASS_WIRELESS_CONTROLLER,	"WIRELESS_CONTROLLER"},
    {IF_CLASS_MISCELLANEOUS,		"MISCELLANEOUS"},
    {IF_CLASS_APPLICATION_SPECIFIC,	"APPLICATION_SPECIFIC"},
    {IF_CLASS_VENDOR_SPECIFIC,		"VENDOR_SPECIFIC"},
    {0, NULL}
};


static const value_string usb_transfer_type_vals[] = {
    {URB_CONTROL, "URB_CONTROL"},
    {URB_ISOCHRONOUS,"URB_ISOCHRONOUS"},
    {URB_INTERRUPT,"URB_INTERRUPT"},
    {URB_BULK,"URB_BULK"},
    {0, NULL}
};

static const value_string usb_urb_type_vals[] = {
    {URB_SUBMIT, "URB_SUBMIT"},
    {URB_COMPLETE,"URB_COMPLETE"},
    {URB_ERROR,"URB_ERROR"},
    {0, NULL}
};

#define USB_DT_DEVICE		1
#define USB_DT_CONFIGURATION	2
#define USB_DT_STRING		3
#define USB_DT_INTERFACE	4
#define USB_DT_ENDPOINT		5
#define USB_DT_DEVICE_QUALIFIER	6
#define USB_DT_OTHER_SPEED_CONFIGURATION	7
#define USB_DT_INTERFACE_POWER	8
#define USB_DT_HID		0x21
static const value_string descriptor_type_vals[] = {
    {USB_DT_DEVICE,			"DEVICE"},
    {USB_DT_CONFIGURATION,		"CONFIGURATION"},
    {USB_DT_STRING,			"STRING"},
    {USB_DT_INTERFACE,			"INTERFACE"},
    {USB_DT_ENDPOINT,			"ENDPOINT"},
    {USB_DT_DEVICE_QUALIFIER,		"DEVICE_QUALIFIER"},
    {USB_DT_OTHER_SPEED_CONFIGURATION,	"OTHER_SPEED_CONFIGURATION"},
    {USB_DT_INTERFACE_POWER,		"INTERFACE_POWER"},
    {USB_DT_HID,			"HID"},
    {0,NULL}
};

static const value_string usb_bmAttributes_transfer_vals[] = {
    {0x00,	"Control-Transfer"},
    {0x01,	"Isochronous-Transfer"},
    {0x02,	"Bulk-Transfer"},
    {0x03,	"Interrupt-Transfer"},
    {0,NULL}
};

static const value_string usb_bmAttributes_sync_vals[] = {
    {0x00,	"No Sync"},
    {0x04,	"Asynchronous"},
    {0x08,	"Adaptive"},
    {0x0c,	"Synchronous"},
    {0,NULL}
};

static const value_string usb_bmAttributes_behaviour_vals[] = {
    {0x00,	"Data-Endpoint"},
    {0x10,	"Explicit Feedback-Endpoint"},
    {0x20,	"Implicit Feedback-Data-Endpoint"},
    {0x30,	"Reserved"},
    {0,NULL}
};


static usb_conv_info_t *
get_usb_conv_info(conversation_t *conversation)
{
    usb_conv_info_t *usb_conv_info;

    /* do we have conversation specific data ? */
    usb_conv_info = conversation_get_proto_data(conversation, proto_usb);
    if(!usb_conv_info){
        /* no not yet so create some */
        usb_conv_info = se_alloc(sizeof(usb_conv_info_t));
        usb_conv_info->interfaceClass=IF_CLASS_UNKNOWN;
        usb_conv_info->transactions=se_tree_create_non_persistent(EMEM_TREE_TYPE_RED_BLACK, "usb transactions");
        usb_conv_info->class_data=NULL;

        conversation_add_proto_data(conversation, proto_usb, usb_conv_info);
    }
 
    return usb_conv_info;
}  

static conversation_t *
get_usb_conversation(packet_info *pinfo, address *src_addr, address *dst_addr, guint32 src_endpoint, guint32 dst_endpoint)
{
    conversation_t *conversation;

    /*
     * Do we have a conversation for this connection?
     */
    conversation = find_conversation(pinfo->fd->num, 
                               src_addr, dst_addr,
                               pinfo->ptype, 
                               src_endpoint, dst_endpoint, 0);
    if(conversation){
        return conversation;
    }

    /* We don't yet have a conversation, so create one. */
    conversation = conversation_new(pinfo->fd->num, 
                           src_addr, dst_addr,
                           pinfo->ptype,
                           src_endpoint, dst_endpoint, 0);
    return conversation;
}



/* SETUP dissectors */


/*
 * This dissector is used to dissect the setup part and the data
 * for URB_CONTROL_INPUT / GET DESCRIPTOR
 */


/* 9.6.2 */
static int
dissect_usb_device_qualifier_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_trans_info_t *usb_trans_info _U_, usb_conv_info_t *usb_conv_info _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset=offset;

    if(parent_tree){
        item=proto_tree_add_text(parent_tree, tvb, offset, 0, "DEVICE QUALIFIER DESCRIPTOR");
        tree=proto_item_add_subtree(item, ett_descriptor_device);
    }

    /* bLength */
    proto_tree_add_item(tree, hf_usb_bLength, tvb, offset, 1, TRUE);
    offset++;

    /* bDescriptorType */
    proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, TRUE);
    offset++;

    /* bcdUSB */
    proto_tree_add_item(tree, hf_usb_bcdUSB, tvb, offset, 2, TRUE);
    offset+=2;

    /* bDeviceClass */
    proto_tree_add_item(tree, hf_usb_bDeviceClass, tvb, offset, 1, TRUE);
    offset++;

    /* bDeviceSubClass */
    proto_tree_add_item(tree, hf_usb_bDeviceSubClass, tvb, offset, 1, TRUE);
    offset++;

    /* bDeviceProtocol */
    proto_tree_add_item(tree, hf_usb_bDeviceProtocol, tvb, offset, 1, TRUE);
    offset++;

    /* bMaxPacketSize0 */
    proto_tree_add_item(tree, hf_usb_bMaxPacketSize0, tvb, offset, 1, TRUE);
    offset++;

    /* bNumConfigurations */
    proto_tree_add_item(tree, hf_usb_bNumConfigurations, tvb, offset, 1, TRUE);
    offset++;

    /* one reserved byte */
    offset++;

    if(item){
        proto_item_set_len(item, offset-old_offset);
    }

    return offset;
}

/* 9.6.1 */
static int
dissect_usb_device_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_trans_info_t *usb_trans_info _U_, usb_conv_info_t *usb_conv_info _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset=offset;

    if(parent_tree){
        item=proto_tree_add_text(parent_tree, tvb, offset, 0, "DEVICE DESCRIPTOR");
        tree=proto_item_add_subtree(item, ett_descriptor_device);
    }

    /* bLength */
    proto_tree_add_item(tree, hf_usb_bLength, tvb, offset, 1, TRUE);
    offset++;

    /* bDescriptorType */
    proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, TRUE);
    offset++;

    /* bcdUSB */
    proto_tree_add_item(tree, hf_usb_bcdUSB, tvb, offset, 2, TRUE);
    offset+=2;

    /* bDeviceClass */
    proto_tree_add_item(tree, hf_usb_bDeviceClass, tvb, offset, 1, TRUE);
    offset++;

    /* bDeviceSubClass */
    proto_tree_add_item(tree, hf_usb_bDeviceSubClass, tvb, offset, 1, TRUE);
    offset++;

    /* bDeviceProtocol */
    proto_tree_add_item(tree, hf_usb_bDeviceProtocol, tvb, offset, 1, TRUE);
    offset++;

    /* bMaxPacketSize0 */
    proto_tree_add_item(tree, hf_usb_bMaxPacketSize0, tvb, offset, 1, TRUE);
    offset++;

    /* idVendor */
    proto_tree_add_item(tree, hf_usb_idVendor, tvb, offset, 2, TRUE);
    offset+=2;

    /* idProduct */
    proto_tree_add_item(tree, hf_usb_idProduct, tvb, offset, 2, TRUE);
    offset+=2;

    /* bcdDevice */
    proto_tree_add_item(tree, hf_usb_bcdDevice, tvb, offset, 2, TRUE);
    offset+=2;

    /* iManufacturer */
    proto_tree_add_item(tree, hf_usb_iManufacturer, tvb, offset, 1, TRUE);
    offset++;

    /* iProduct */
    proto_tree_add_item(tree, hf_usb_iProduct, tvb, offset, 1, TRUE);
    offset++;

    /* iSerialNumber */
    proto_tree_add_item(tree, hf_usb_iSerialNumber, tvb, offset, 1, TRUE);
    offset++;

    /* bNumConfigurations */
    proto_tree_add_item(tree, hf_usb_bNumConfigurations, tvb, offset, 1, TRUE);
    offset++;

    if(item){
        proto_item_set_len(item, offset-old_offset);
    }

    return offset;
}

/* 9.6.7 */
static int
dissect_usb_string_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_trans_info_t *usb_trans_info, usb_conv_info_t *usb_conv_info _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset=offset;
    guint8 len;

    if(parent_tree){
        item=proto_tree_add_text(parent_tree, tvb, offset, 0, "STRING DESCRIPTOR");
        tree=proto_item_add_subtree(item, ett_descriptor_device);
    }

    /* bLength */
    proto_tree_add_item(tree, hf_usb_bLength, tvb, offset, 1, TRUE);
    len=tvb_get_guint8(tvb, offset);
    offset++;

    /* bDescriptorType */
    proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, TRUE);
    offset++;

    if(!usb_trans_info->u.get_descriptor.index){
        /* list of languanges */
        while(len>(offset-old_offset)){
            /* wLANGID */
            proto_tree_add_item(tree, hf_usb_wLANGID, tvb, offset, 2, TRUE);
            offset+=2;
        }
    } else {
        char *str;        

        /* unicode string */
        str=tvb_get_ephemeral_faked_unicode(tvb, offset, (len-2)/2, TRUE);
        proto_tree_add_string(tree, hf_usb_bString, tvb, offset, len-2, str);
    }

    if(item){
        proto_item_set_len(item, offset-old_offset);
    }

    return offset;
}



/* 9.6.5 */
static int
dissect_usb_interface_descriptor(packet_info *pinfo, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_trans_info_t *usb_trans_info, usb_conv_info_t *usb_conv_info)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset=offset;

    if(parent_tree){
        item=proto_tree_add_text(parent_tree, tvb, offset, 0, "INTERFACE DESCRIPTOR");
        tree=proto_item_add_subtree(item, ett_descriptor_device);
    }

    /* bLength */
    proto_tree_add_item(tree, hf_usb_bLength, tvb, offset, 1, TRUE);
    offset++;

    /* bDescriptorType */
    proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, TRUE);
    offset++;

    /* bInterfaceNumber */
    proto_tree_add_item(tree, hf_usb_bInterfaceNumber, tvb, offset, 1, TRUE);
    offset++;

    /* bAlternateSetting */
    proto_tree_add_item(tree, hf_usb_bAlternateSetting, tvb, offset, 1, TRUE);
    offset++;

    /* bNumEndpoints */
    proto_tree_add_item(tree, hf_usb_bNumEndpoints, tvb, offset, 1, TRUE);
    offset++;

    /* bInterfaceClass */
    proto_tree_add_item(tree, hf_usb_bInterfaceClass, tvb, offset, 1, TRUE);
    /* save the class so we can access it later in the endpoint descriptor */
    usb_conv_info->interfaceClass=tvb_get_guint8(tvb, offset);
    if(!pinfo->fd->flags.visited){
        usb_trans_info->interface_info=se_alloc(sizeof(usb_conv_info_t));
        usb_trans_info->interface_info->interfaceClass=tvb_get_guint8(tvb, offset);
        usb_trans_info->interface_info->transactions=se_tree_create_non_persistent(EMEM_TREE_TYPE_RED_BLACK, "usb transactions");
    }
    offset++;

    /* bInterfaceSubClass */
    proto_tree_add_item(tree, hf_usb_bInterfaceSubClass, tvb, offset, 1, TRUE);
    offset++;

    /* bInterfaceProtocol */
    proto_tree_add_item(tree, hf_usb_bInterfaceProtocol, tvb, offset, 1, TRUE);
    offset++;

    /* iInterface */
    proto_tree_add_item(tree, hf_usb_iInterface, tvb, offset, 1, TRUE);
    offset++;

    if(item){
        proto_item_set_len(item, offset-old_offset);
    }

    return offset;
}

/* 9.6.6 */
static const true_false_string tfs_endpoint_direction = {
    "IN Endpoint",
    "OUT Endpoint"
};
static int
dissect_usb_endpoint_descriptor(packet_info *pinfo, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_trans_info_t *usb_trans_info _U_, usb_conv_info_t *usb_conv_info _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    proto_item *endpoint_item=NULL;
    proto_tree *endpoint_tree=NULL;
    proto_item *ep_attrib_item=NULL;
    proto_tree *ep_attrib_tree=NULL;
    int old_offset=offset;
    guint8 endpoint;

    if(parent_tree){
        item=proto_tree_add_text(parent_tree, tvb, offset, 0, "ENDPOINT DESCRIPTOR");
        tree=proto_item_add_subtree(item, ett_descriptor_device);
    }

    /* bLength */
    proto_tree_add_item(tree, hf_usb_bLength, tvb, offset, 1, TRUE);
    offset++;

    /* bDescriptorType */
    proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, TRUE);
    offset++;

    /* bEndpointAddress */
    if(tree){
        endpoint_item=proto_tree_add_item(tree, hf_usb_bEndpointAddress, tvb, offset, 1, TRUE);
        endpoint_tree=proto_item_add_subtree(endpoint_item, ett_configuration_bEndpointAddress);
    }
    endpoint=tvb_get_guint8(tvb, offset)&0x0f;
    proto_tree_add_item(endpoint_tree, hf_usb_bEndpointAddress_direction, tvb, offset, 1, TRUE);
    proto_item_append_text(endpoint_item, "  %s", (tvb_get_guint8(tvb, offset)&0x80)?"IN":"OUT");
    proto_tree_add_item(endpoint_tree, hf_usb_bEndpointAddress_number, tvb, offset, 1, TRUE);
    proto_item_append_text(endpoint_item, "  Endpoint:%d", endpoint);
    offset++;

    /* Together with class from the interface descriptor we know what kind
     * of class the device at endpoint is.
     * Make sure a conversation exists for this endpoint and attach a 
     * usb_conv_into_t structure to it.
     *
     * All endpoints for the same interface descriptor share the same
     * usb_conv_info structure.
     */
    if((!pinfo->fd->flags.visited)&&usb_trans_info->interface_info){
        conversation_t *conversation;

        if(pinfo->destport==NO_ENDPOINT){
            static address tmp_addr;
            static usb_address_t usb_addr;

            /* Create a new address structure that points to the same device
             * but the new endpoint.
             */
            usb_addr.device=((usb_address_t *)(pinfo->src.data))->device;
            usb_addr.endpoint=endpoint;
            SET_ADDRESS(&tmp_addr, AT_USB, USB_ADDR_LEN, (char *)&usb_addr);
            conversation=get_usb_conversation(pinfo, &tmp_addr, &pinfo->dst, endpoint, pinfo->destport);
        } else {
            static address tmp_addr;
            static usb_address_t usb_addr;

            /* Create a new address structure that points to the same device
             * but the new endpoint.
             */
            usb_addr.device=((usb_address_t *)(pinfo->dst.data))->device;
            usb_addr.endpoint=endpoint;
            SET_ADDRESS(&tmp_addr, AT_USB, USB_ADDR_LEN, (char *)&usb_addr);
            conversation=get_usb_conversation(pinfo, &pinfo->src, &tmp_addr, pinfo->srcport, endpoint);
        }

        conversation_add_proto_data(conversation, proto_usb, usb_trans_info->interface_info);
    }

    /* bmAttributes */
    if (tree) {
        ep_attrib_item=proto_tree_add_item(tree, hf_usb_bmAttributes, tvb, offset, 1, TRUE);
	ep_attrib_tree=proto_item_add_subtree(ep_attrib_item, ett_endpoint_bmAttributes);
    }
    proto_tree_add_item(ep_attrib_tree, hf_usb_bEndpointAttributeTransfer, tvb, offset, 1, TRUE);
    /* isochronous only */
    proto_tree_add_item(ep_attrib_tree, hf_usb_bEndpointAttributeSynchonisation, tvb, offset, 1, TRUE);
    /* isochronous only */
    proto_tree_add_item(ep_attrib_tree, hf_usb_bEndpointAttributeBehaviour, tvb, offset, 1, TRUE);
    offset++;

    /* wMaxPacketSize */
    proto_tree_add_item(tree, hf_usb_wMaxPacketSize, tvb, offset, 2, TRUE);
    offset+=2;

    /* bInterval */
    proto_tree_add_item(tree, hf_usb_bInterval, tvb, offset, 1, TRUE);
    offset++;

    if(item){
        proto_item_set_len(item, offset-old_offset);
    }

    return offset;
}

static int
dissect_usb_unknown_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_trans_info_t *usb_trans_info _U_, usb_conv_info_t *usb_conv_info _U_)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset=offset;
    guint8 bLength;

    if(parent_tree){
        item=proto_tree_add_text(parent_tree, tvb, offset, 0, "UNKNOWN DESCRIPTOR");
        tree=proto_item_add_subtree(item, ett_descriptor_device);
    }

    /* bLength */
    proto_tree_add_item(tree, hf_usb_bLength, tvb, offset, 1, TRUE);
    bLength = tvb_get_guint8(tvb, offset);
    offset++;

    /* bDescriptorType */
    proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, TRUE);
    offset++;

    offset += bLength - 2;

    if(item){
        proto_item_set_len(item, offset-old_offset);
    }

    return offset;
}

/* 9.6.3 */
static const true_false_string tfs_mustbeone = {
    "Must be 1 for USB 1.1 and higher",
    "FIXME: Is this a USB 1.0 device"
};
static const true_false_string tfs_selfpowered = {
    "This device is SELF-POWERED",
    "This device is powered from the USB bus"
};
static const true_false_string tfs_remotewakeup = {
    "This device supports REMOTE WAKEUP",
    "This device does NOT support remote wakeup"
};
static int
dissect_usb_configuration_descriptor(packet_info *pinfo _U_, proto_tree *parent_tree, tvbuff_t *tvb, int offset, usb_trans_info_t *usb_trans_info, usb_conv_info_t *usb_conv_info)
{
    proto_item *item=NULL;
    proto_tree *tree=NULL;
    int old_offset=offset;
    guint16 len;
    proto_item *flags_item=NULL;
    proto_tree *flags_tree=NULL;
    guint8 flags;
    proto_item *power_item=NULL;
    guint8 power;

    if(parent_tree){
        item=proto_tree_add_text(parent_tree, tvb, offset, 0, "CONFIGURATION DESCRIPTOR");
        tree=proto_item_add_subtree(item, ett_descriptor_device);
    }

    /* bLength */
    proto_tree_add_item(tree, hf_usb_bLength, tvb, offset, 1, TRUE);
    offset++;

    /* bDescriptorType */
    proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, TRUE);
    offset++;

    /* wTotalLength */
    proto_tree_add_item(tree, hf_usb_wTotalLength, tvb, offset, 2, TRUE);
    len=tvb_get_letohs(tvb, offset);
    offset+=2;

    /* bNumInterfaces */
    proto_tree_add_item(tree, hf_usb_bNumInterfaces, tvb, offset, 1, TRUE);
    offset++;

    /* bConfigurationValue */
    proto_tree_add_item(tree, hf_usb_bConfigurationValue, tvb, offset, 1, TRUE);
    offset++;

    /* iConfiguration */
    proto_tree_add_item(tree, hf_usb_iConfiguration, tvb, offset, 1, TRUE);
    offset++;

    /* bmAttributes */
    if(tree){
        flags_item=proto_tree_add_item(tree, hf_usb_configuration_bmAttributes, tvb, offset, 1, TRUE);
        flags_tree=proto_item_add_subtree(flags_item, ett_configuration_bmAttributes);
    }
    flags=tvb_get_guint8(tvb, offset);
    proto_tree_add_item(flags_tree, hf_usb_configuration_legacy10buspowered, tvb, offset, 1, TRUE);
    proto_tree_add_item(flags_tree, hf_usb_configuration_selfpowered, tvb, offset, 1, TRUE);
    proto_item_append_text(flags_item, "  %sSELF-POWERED", (flags&0x40)?"":"NOT ");
    flags&=~0x40;
    proto_tree_add_item(flags_tree, hf_usb_configuration_remotewakeup, tvb, offset, 1, TRUE);
    proto_item_append_text(flags_item, "  %sREMOTE-WAKEUP", (flags&0x20)?"":"NO ");
    flags&=~0x20;
    offset++;

    /* bMaxPower */
    power_item=proto_tree_add_item(tree, hf_usb_bMaxPower, tvb, offset, 1, TRUE);
    power=tvb_get_guint8(tvb, offset);
    proto_item_append_text(power_item, "  (%dmA)", power*2);
    offset++;

    /* initialize interface_info to NULL */
    usb_trans_info->interface_info=NULL;

    /* decode any additional interface and endpoint descriptors */
    while(len>(old_offset-offset)){
        guint8 next_type;

        if(tvb_length_remaining(tvb, offset)<2){
            break;
        }
        next_type=tvb_get_guint8(tvb, offset+1);
        switch(next_type){
        case USB_DT_INTERFACE:
            offset=dissect_usb_interface_descriptor(pinfo, parent_tree, tvb, offset, usb_trans_info, usb_conv_info);
            break;
        case USB_DT_ENDPOINT:
            offset=dissect_usb_endpoint_descriptor(pinfo, parent_tree, tvb, offset, usb_trans_info, usb_conv_info);
            break;
        default:
            offset=dissect_usb_unknown_descriptor(pinfo, parent_tree, tvb, offset, usb_trans_info, usb_conv_info);
            break;
            /* was: return offset; */
        }
    }

    if(item){
        proto_item_set_len(item, offset-old_offset);
    }

    return offset;
}


static void
dissect_usb_setup_get_descriptor(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean is_request, usb_trans_info_t *usb_trans_info, usb_conv_info_t *usb_conv_info)
{
    if(is_request){
        /* descriptor index */
        proto_tree_add_item(tree, hf_usb_descriptor_index, tvb, offset, 1, TRUE);
        usb_trans_info->u.get_descriptor.index=tvb_get_guint8(tvb, offset);
        offset++;

        /* descriptor type */
        proto_tree_add_item(tree, hf_usb_bDescriptorType, tvb, offset, 1, TRUE);
        usb_trans_info->u.get_descriptor.type=tvb_get_guint8(tvb, offset);
        offset++;
        if (check_col(pinfo->cinfo, COL_INFO)) {
            col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
                val_to_str(usb_trans_info->u.get_descriptor.type, descriptor_type_vals, "Unknown type %x"));
        }

        /* language id */
        proto_tree_add_item(tree, hf_usb_language_id, tvb, offset, 2, TRUE);
        offset+=2;

        /* length */
        proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, TRUE);
        offset += 2;
    } else {
        if (check_col(pinfo->cinfo, COL_INFO)) {
            col_append_fstr(pinfo->cinfo, COL_INFO, " %s",
                val_to_str(usb_trans_info->u.get_descriptor.type, descriptor_type_vals, "Unknown type %x"));
        }
        switch(usb_trans_info->u.get_descriptor.type){
        case USB_DT_DEVICE:
            offset=dissect_usb_device_descriptor(pinfo, tree, tvb, offset, usb_trans_info, usb_conv_info);
            break;
        case USB_DT_CONFIGURATION:
            offset=dissect_usb_configuration_descriptor(pinfo, tree, tvb, offset, usb_trans_info, usb_conv_info);
            break;
        case USB_DT_STRING: 
            offset=dissect_usb_string_descriptor(pinfo, tree, tvb, offset, usb_trans_info, usb_conv_info);
            break;
        case USB_DT_INTERFACE:
            offset=dissect_usb_interface_descriptor(pinfo, tree, tvb, offset, usb_trans_info, usb_conv_info);
            break;
        case USB_DT_ENDPOINT:
            offset=dissect_usb_endpoint_descriptor(pinfo, tree, tvb, offset, usb_trans_info, usb_conv_info);
            break;
        case USB_DT_DEVICE_QUALIFIER:
            offset=dissect_usb_device_qualifier_descriptor(pinfo, tree, tvb, offset, usb_trans_info, usb_conv_info);
            break;
        default:
            /* XXX dissect the descriptor coming back from the device */
            proto_tree_add_text(tree, tvb, offset, tvb_length_remaining(tvb, offset), "get descriptor  data...");
        }
    }
}




typedef void (*usb_setup_dissector)(packet_info *pinfo, proto_tree *tree, tvbuff_t *tvb, int offset, gboolean is_request, usb_trans_info_t *usb_trans_info, usb_conv_info_t *usb_conv_info);

typedef struct _usb_setup_dissector_table_t {
    guint8 request;
    usb_setup_dissector dissector;
} usb_setup_dissector_table_t;
#define USB_SETUP_GET_DESCRIPTOR	6
static const usb_setup_dissector_table_t setup_dissectors[] = {
    {USB_SETUP_GET_DESCRIPTOR,	dissect_usb_setup_get_descriptor},
    {0, NULL}
};  
static const value_string setup_request_names_vals[] = {
    {USB_SETUP_GET_DESCRIPTOR,		"GET DESCRIPTOR"},
    {0, NULL}
};  








static const true_false_string tfs_bmrequesttype_direction = {
	"Device-to-host",
	"Host-to-device"
};

#define RQT_SETUP_TYPE_STANDARD	0
#define RQT_SETUP_TYPE_CLASS	1
#define RQT_SETUP_TYPE_VENDOR	2

static const value_string bmrequesttype_type_vals[] = {
    {RQT_SETUP_TYPE_STANDARD, "Standard"},
    {RQT_SETUP_TYPE_CLASS,    "Class"},
    {RQT_SETUP_TYPE_VENDOR,   "Vendor"},
    {0, NULL}
};
static const value_string bmrequesttype_recipient_vals[] = {
    {0, "Device"},
    {1, "Interface"},
    {2, "Endpoint"},
    {3, "Other"},
    {0, NULL}
};

static int
dissect_usb_bmrequesttype(proto_tree *parent_tree, tvbuff_t *tvb, int offset,
    int *type)
{
	proto_item *item=NULL;
	proto_tree *tree=NULL;
	guint8 bmRequestType;

	if(parent_tree){
	        item=proto_tree_add_item(parent_tree, hf_usb_bmRequestType, tvb, offset, 1, TRUE);
		tree = proto_item_add_subtree(item, ett_usb_setup_bmrequesttype);
	}

	bmRequestType = tvb_get_guint8(tvb, offset);
	*type = (bmRequestType >> 5) & 0x03;
	proto_tree_add_item(tree, hf_usb_bmRequestType_direction, tvb, offset, 1, TRUE);
	proto_tree_add_item(tree, hf_usb_bmRequestType_type, tvb, offset, 1, TRUE);
	proto_tree_add_item(tree, hf_usb_bmRequestType_recipient, tvb, offset, 1, TRUE);

	offset++;
	return offset;
}





static void
dissect_linux_usb(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent)
{
    int offset = 0;
    int type, endpoint;
    guint8 setup_flag;
    proto_tree *tree = NULL;
    guint32 src_device, dst_device, tmp_addr;
    static usb_address_t src_addr, dst_addr; /* has to be static due to SET_ADDRESS */
    guint32 src_endpoint, dst_endpoint;
    gboolean is_request=FALSE;
    usb_conv_info_t *usb_conv_info=NULL;
    usb_trans_info_t *usb_trans_info=NULL;
    conversation_t *conversation;
    usb_tap_data_t *tap_data=NULL;
    
    if (check_col(pinfo->cinfo, COL_PROTOCOL))
        col_set_str(pinfo->cinfo, COL_PROTOCOL, "USB");

    /* add usb hdr*/    
    if (parent) {
      proto_item *ti = NULL;
      ti = proto_tree_add_protocol_format(parent, proto_usb, tvb, 0, sizeof(struct usb_request_hdr), "USB URB");

      tree = proto_item_add_subtree(ti, usb_hdr);
    }

    proto_tree_add_uint64(tree, hf_usb_urb_id, tvb, 0, 0,
                          pinfo->pseudo_header->linux_usb.id);

    proto_tree_add_uint(tree, hf_usb_urb_type, tvb, 0, 0,
                        pinfo->pseudo_header->linux_usb.event_type);

    type = pinfo->pseudo_header->linux_usb.transfer_type;
    proto_tree_add_uint(tree, hf_usb_transfer_type, tvb, 0, 0, type);
    if (check_col(pinfo->cinfo, COL_INFO)) {
        col_append_fstr(pinfo->cinfo, COL_INFO, "%s",
            val_to_str(type, usb_transfer_type_vals, "Unknown type %x"));
    }

#if 0
    /* The direction flag is broken so we must strip it off */
    endpoint=pinfo->pseudo_header->linux_usb.endpoint_number;
#else
    endpoint=pinfo->pseudo_header->linux_usb.endpoint_number&(~URB_TRANSFER_IN);
#endif
    proto_tree_add_uint(tree, hf_usb_endpoint_number, tvb, 0, 0, endpoint);

    tmp_addr=pinfo->pseudo_header->linux_usb.device_address;
    proto_tree_add_uint(tree, hf_usb_device_address, tvb, 0, 0, tmp_addr);

    proto_tree_add_uint(tree, hf_usb_bus_id, tvb, 0, 0,
                        pinfo->pseudo_header->linux_usb.bus_id);

    setup_flag = pinfo->pseudo_header->linux_usb.setup_flag;
    proto_tree_add_uint(tree, hf_usb_setup_flag, tvb, 0, 0, setup_flag);

    proto_tree_add_uint(tree, hf_usb_data_flag, tvb, 0, 0,
                        pinfo->pseudo_header->linux_usb.data_flag);

#if 0
    /* this is how it is supposed to work but this flag seems to be broken -- ronnie */
    is_request = endpoint & URB_TRANSFER_IN;
#else
    /* Determine whether this is a request or a response */
    switch(type){
    case URB_BULK:
    case URB_CONTROL:
        switch(pinfo->pseudo_header->linux_usb.event_type){
        case URB_SUBMIT:
            is_request=TRUE;
            break;
        case URB_COMPLETE:
        case URB_ERROR:
            is_request=FALSE;
            break;
        default:
            DISSECTOR_ASSERT_NOT_REACHED();
        }
        break;
    case URB_INTERRUPT:
        switch(pinfo->pseudo_header->linux_usb.event_type){
        case URB_SUBMIT:
            is_request=FALSE;
            break;
        case URB_COMPLETE:
        case URB_ERROR:
            is_request=TRUE;
            break;
        default:
            DISSECTOR_ASSERT_NOT_REACHED();
        }
        break;
     default:
        DISSECTOR_ASSERT_NOT_REACHED();
    }
#endif

    /* Set up addresses and ports.
     * Note that URB_INTERRUPT goes in the reverse direction and thus
     * the request comes from the device and not the host.
     */
    if ( (is_request&&(type!=URB_INTERRUPT))
      || (!is_request&&(type==URB_INTERRUPT)) ){
        src_addr.device = src_device = 0xffffffff;
        src_addr.endpoint = src_endpoint = NO_ENDPOINT;
        dst_addr.device = dst_device = htolel(tmp_addr);
        dst_addr.endpoint = dst_endpoint = htolel(endpoint);
    } else {
        src_addr.device = src_device = htolel(tmp_addr);
        src_addr.endpoint = src_endpoint = htolel(endpoint);
        dst_addr.device = dst_device = 0xffffffff;
        dst_addr.endpoint = dst_endpoint = NO_ENDPOINT;
    }

    SET_ADDRESS(&pinfo->net_src, AT_USB, USB_ADDR_LEN, (char *)&src_addr);
    SET_ADDRESS(&pinfo->src, AT_USB, USB_ADDR_LEN, (char *)&src_addr);
    SET_ADDRESS(&pinfo->net_dst, AT_USB, USB_ADDR_LEN, (char *)&dst_addr);
    SET_ADDRESS(&pinfo->dst, AT_USB, USB_ADDR_LEN, (char *)&dst_addr);
    pinfo->ptype=PT_USB;
    pinfo->srcport=src_endpoint;
    pinfo->destport=dst_endpoint;

    conversation=get_usb_conversation(pinfo, &pinfo->src, &pinfo->dst, pinfo->srcport, pinfo->destport);

    usb_conv_info=get_usb_conv_info(conversation);
    pinfo->usb_conv_info=usb_conv_info;

   
    /* request/response matching so we can keep track of transaction specific
     * data.
     */
    if(is_request){
        /* this is a request */
        usb_trans_info=se_tree_lookup32(usb_conv_info->transactions, pinfo->fd->num);
        if(!usb_trans_info){
            usb_trans_info=se_alloc(sizeof(usb_trans_info_t));
            usb_trans_info->request_in=pinfo->fd->num;
            usb_trans_info->response_in=0;
            usb_trans_info->req_time=pinfo->fd->abs_ts;
            usb_trans_info->requesttype=0;
            usb_trans_info->request=0;
            se_tree_insert32(usb_conv_info->transactions, pinfo->fd->num, usb_trans_info);
        }
        usb_conv_info->usb_trans_info=usb_trans_info;

        if(usb_trans_info && usb_trans_info->response_in){
            proto_item *ti;

            ti=proto_tree_add_uint(tree, hf_usb_response_in, tvb, 0, 0, usb_trans_info->response_in);
            PROTO_ITEM_SET_GENERATED(ti);
        }
    } else {
        /* this is a response */
        if(pinfo->fd->flags.visited){
            usb_trans_info=se_tree_lookup32(usb_conv_info->transactions, pinfo->fd->num);
        } else {
            usb_trans_info=se_tree_lookup32_le(usb_conv_info->transactions, pinfo->fd->num);
            if(usb_trans_info){
                usb_trans_info->response_in=pinfo->fd->num;
                se_tree_insert32(usb_conv_info->transactions, pinfo->fd->num, usb_trans_info);
            }
        } 
        usb_conv_info->usb_trans_info=usb_trans_info;

        if(usb_trans_info && usb_trans_info->request_in){
            proto_item *ti;
            nstime_t t, deltat;

            ti=proto_tree_add_uint(tree, hf_usb_request_in, tvb, 0, 0, usb_trans_info->request_in);
            PROTO_ITEM_SET_GENERATED(ti);

            t = pinfo->fd->abs_ts;
            nstime_delta(&deltat, &t, &usb_trans_info->req_time);
            ti=proto_tree_add_time(tree, hf_usb_time, tvb, 0, 0, &deltat);
            PROTO_ITEM_SET_GENERATED(ti);
        }
    }
    
    /* For DLT189 it seems 
     * that all INTERRUPT or BULK packets as well as all CONTROL responses
     * are prepended with 8 mysterious bytes.
     */
    switch(type){
    case URB_CONTROL:
        if(pinfo->pseudo_header->linux_usb.event_type!=URB_SUBMIT){
            offset+=8;
        }
        break;
    case URB_BULK:
    case URB_INTERRUPT:
        offset+=8;
        break;
    default:
        DISSECTOR_ASSERT_NOT_REACHED();
    }

    tap_data=ep_alloc(sizeof(usb_tap_data_t));
    tap_data->urb_type=(guint8)pinfo->pseudo_header->linux_usb.event_type;
    tap_data->transfer_type=(guint8)type;
    tap_data->conv_info=usb_conv_info;
    tap_data->trans_info=usb_trans_info;
    tap_queue_packet(usb_tap, pinfo, tap_data);


    switch(type){
    case URB_BULK:
        {
        proto_item *item;

        item=proto_tree_add_uint(tree, hf_usb_bInterfaceClass, tvb, 0, 0, usb_conv_info->interfaceClass);
        PROTO_ITEM_SET_GENERATED(item);
        if(tvb_length_remaining(tvb, offset)){
            tvbuff_t *next_tvb;

            pinfo->usb_conv_info=usb_conv_info;
            next_tvb=tvb_new_subset(tvb, offset, -1, -1);
            if(dissector_try_port(usb_bulk_dissector_table, usb_conv_info->interfaceClass, next_tvb, pinfo, parent)){
                return;
            }
        }
        }
        break;
    case URB_CONTROL:
        {
        const usb_setup_dissector_table_t *tmp;
        usb_setup_dissector dissector;
        proto_item *ti = NULL;
        proto_tree *setup_tree = NULL;
        int type;
 
        ti=proto_tree_add_uint(tree, hf_usb_bInterfaceClass, tvb, offset, 0, usb_conv_info->interfaceClass);
        PROTO_ITEM_SET_GENERATED(ti);

        if(is_request){
            if (setup_flag == 0) {
                tvbuff_t *next_tvb;

                /* this is a request */
                ti = proto_tree_add_protocol_format(tree, proto_usb, tvb, offset, sizeof(struct usb_request_hdr), "URB setup");
                setup_tree = proto_item_add_subtree(ti, usb_setup_hdr);
                usb_trans_info->requesttype=tvb_get_guint8(tvb, offset);        
                offset=dissect_usb_bmrequesttype(setup_tree, tvb, offset, &type);


                /* read the request code and spawn off to a class specific 
                 * dissector if found 
                 */
                usb_trans_info->request=tvb_get_guint8(tvb, offset);

                switch (type) {

                case RQT_SETUP_TYPE_STANDARD:
                    /* 
                     * This is a standard request which is managed by this
                     * dissector
                     */
                    proto_tree_add_item(setup_tree, hf_usb_request, tvb, offset, 1, TRUE);
                    offset += 1;

                    if (check_col(pinfo->cinfo, COL_INFO)) {
                        col_clear(pinfo->cinfo, COL_INFO);
                        col_append_fstr(pinfo->cinfo, COL_INFO, "%s Request",
                             val_to_str(usb_trans_info->request, setup_request_names_vals, "Unknown type %x"));
                    }

                    dissector=NULL;
                    for(tmp=setup_dissectors;tmp->dissector;tmp++){
                        if(tmp->request==usb_trans_info->request){
                            dissector=tmp->dissector;
                            break;
                        }
                    }
  
                    if(dissector){
                        dissector(pinfo, setup_tree, tvb, offset, is_request, usb_trans_info, usb_conv_info);
                        offset+=6;
                    } else {
                        proto_tree_add_item(setup_tree, hf_usb_value, tvb, offset, 2, TRUE);
                        offset += 2;
                        proto_tree_add_item(setup_tree, hf_usb_index, tvb, offset, 2, TRUE);
                        offset += 2;
                        proto_tree_add_item(setup_tree, hf_usb_length, tvb, offset, 2, TRUE);
                        offset += 2;
                    }
                    break;

                case RQT_SETUP_TYPE_CLASS:
                    /* Try to find a class specific dissector */  
                    next_tvb=tvb_new_subset(tvb, offset, -1, -1);
                    if(dissector_try_port(usb_control_dissector_table, usb_conv_info->interfaceClass, next_tvb, pinfo, tree)){
                        return;
                    /* XXX - dump as hex */
                    }
                    break;
                }
            } else {
                offset += 8;
            }
        } else {
            tvbuff_t *next_tvb;

            /* this is a response */
            if(usb_conv_info->usb_trans_info){
                /* Try to find a class specific dissector */  
                next_tvb=tvb_new_subset(tvb, offset, -1, -1);
                if(dissector_try_port(usb_control_dissector_table, usb_conv_info->interfaceClass, next_tvb, pinfo, tree)){
                    return;
                }

                if (check_col(pinfo->cinfo, COL_INFO)) {
                    col_clear(pinfo->cinfo, COL_INFO);
                    col_append_fstr(pinfo->cinfo, COL_INFO, "%s Response",
                        val_to_str(usb_conv_info->usb_trans_info->request, setup_request_names_vals, "Unknown type %x"));
                }

                dissector=NULL;
                for(tmp=setup_dissectors;tmp->dissector;tmp++){
                    if(tmp->request==usb_conv_info->usb_trans_info->request){
                        dissector=tmp->dissector;
                        break;
                    }
                }

                if(dissector){
                    dissector(pinfo, tree, tvb, offset, is_request, usb_conv_info->usb_trans_info, usb_conv_info);
                }
            } else {
                /* no matching request available */
                ;
            }
        }
        }
        break;
    default:
        /* dont know */
        if (setup_flag == 0) {
            proto_item *ti = NULL;
            proto_tree *setup_tree = NULL;
            guint8 requesttype, request;
            int type;
 
            ti = proto_tree_add_protocol_format(tree, proto_usb, tvb, offset, sizeof(struct usb_request_hdr), "URB setup");
            setup_tree = proto_item_add_subtree(ti, usb_setup_hdr);
 
 
            requesttype=tvb_get_guint8(tvb, offset);
            offset=dissect_usb_bmrequesttype(setup_tree, tvb, offset, &type);
 
            request=tvb_get_guint8(tvb, offset);
            proto_tree_add_item(setup_tree, hf_usb_request, tvb, offset, 1, TRUE);
            offset += 1;
 
            proto_tree_add_item(tree, hf_usb_value, tvb, offset, 2, TRUE);
            offset += 2;
            proto_tree_add_item(tree, hf_usb_index, tvb, offset, 2, TRUE);
            offset += 2;
            proto_tree_add_item(tree, hf_usb_length, tvb, offset, 2, TRUE);
            offset += 2;
        } else {
            offset += 8;
        }
        break;
    }

    proto_tree_add_item(tree, hf_usb_data, tvb, offset, -1, FALSE);
}

void
proto_register_usb(void)
{
    static hf_register_info hf[] = {

        { &hf_usb_urb_id,
        { "URB id", "usb.urb_id", FT_UINT64, BASE_DEC, 
                NULL, 0x0,
                "URB id", HFILL }},

        { &hf_usb_bus_id,
        { "URB bus id", "usb.bus_id", FT_UINT16, BASE_DEC, 
                NULL, 0x0,
                "URB bus id", HFILL }},

        { &hf_usb_urb_type,
        { "URB type", "usb.urb_type", FT_UINT8, BASE_DEC, 
                VALS(usb_urb_type_vals), 0x0,
                "URB type", HFILL }},

        { &hf_usb_transfer_type,
        { "URB transfer type", "usb.transfer_type", FT_UINT8, BASE_DEC, 
                VALS(usb_transfer_type_vals), 0x0,
                "URB transfer type", HFILL }},

        { &hf_usb_device_address,
        { "Device", "usb.device_address", FT_UINT8, BASE_DEC, NULL, 0x0,
                "USB device address", HFILL }},

        { &hf_usb_data_flag,
        { "Data flag", "usb.data_flag", FT_UINT8, BASE_DEC, NULL, 0x0,
                 "USB data flag", HFILL }},

        { &hf_usb_setup_flag,
        { "Setup flag", "usb.setup_flag", FT_UINT8, BASE_DEC, NULL, 0x0,
                 "USB setup flag", HFILL }},

        { &hf_usb_endpoint_number,
        { "Endpoint", "usb.endpoint_number", FT_UINT8, BASE_HEX, NULL, 0x0,
                "USB endpoint number", HFILL }},

        { &hf_usb_src_endpoint_number,
        { "Src Endpoint", "usb.src.endpoint", FT_UINT8, BASE_HEX, NULL, 0x0,
                "Source USB endpoint number", HFILL }},

        { &hf_usb_dst_endpoint_number,
        { "Dst Endpoint", "usb.dst.endpoint", FT_UINT8, BASE_HEX, NULL, 0x0,
                "Destination USB endpoint number", HFILL }},

        { &hf_usb_bmRequestType,
        { "bmRequestType", "usb.bmRequestType", FT_UINT8, BASE_HEX, NULL, 0x0,
                "", HFILL }},

        { &hf_usb_request,
        { "bRequest", "usb.setup.bRequest", FT_UINT8, BASE_HEX, VALS(setup_request_names_vals), 0x0,
                "", HFILL }},

        { &hf_usb_value,
        { "wValue", "usb.setup.wValue", FT_UINT16, BASE_HEX, NULL, 0x0,
                "", HFILL }},

        { &hf_usb_index,
        { "wIndex", "usb.setup.wIndex", FT_UINT16, BASE_DEC, NULL, 0x0,
                "", HFILL }},

        { &hf_usb_length,
        { "wLength", "usb.setup.wLength", FT_UINT16, BASE_DEC, NULL, 0x0,
                "", HFILL }},
                
        { &hf_usb_data,
        {"Application Data", "usb.data",
            FT_BYTES, BASE_HEX, NULL, 0x0,
            "Payload is application data", HFILL }},
    
        { &hf_usb_bmRequestType_direction,
        { "Direction", "usb.bmRequestType.direction", FT_BOOLEAN, 8, 
          TFS(&tfs_bmrequesttype_direction), 0x80, "", HFILL }},

        { &hf_usb_bmRequestType_type,
        { "Type", "usb.bmRequestType.type", FT_UINT8, BASE_HEX, 
          VALS(bmrequesttype_type_vals), 0x60, "", HFILL }},

        { &hf_usb_bmRequestType_recipient,
        { "Recipient", "usb.bmRequestType.recipient", FT_UINT8, BASE_HEX, 
          VALS(bmrequesttype_recipient_vals), 0x1f, "", HFILL }},

        { &hf_usb_bDescriptorType,
        { "bDescriptorType", "usb.bDescriptorType", FT_UINT8, BASE_HEX, 
          VALS(descriptor_type_vals), 0x0, "", HFILL }},

        { &hf_usb_descriptor_index,
        { "Descriptor Index", "usb.DescriptorIndex", FT_UINT8, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_language_id,
        { "Language Id", "usb.LanguageId", FT_UINT16, BASE_HEX, 
          VALS(usb_langid_vals), 0x0, "", HFILL }},

        { &hf_usb_bLength,
        { "bLength", "usb.bLength", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bcdUSB,
        { "bcdUSB", "usb.bcdUSB", FT_UINT16, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bDeviceClass,
        { "bDeviceClass", "usb.bDeviceClass", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bDeviceSubClass,
        { "bDeviceSubClass", "usb.bDeviceSubClass", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bDeviceProtocol,
        { "bDeviceProtocol", "usb.bDeviceProtocol", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bMaxPacketSize0,
        { "bMaxPacketSize0", "usb.bMaxPacketSize0", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_idVendor,
        { "idVendor", "usb.idVendor", FT_UINT16, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_idProduct,
        { "idProduct", "usb.idProduct", FT_UINT16, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bcdDevice,
        { "bcdDevice", "usb.bcdDevice", FT_UINT16, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_iManufacturer,
        { "iManufacturer", "usb.iManufacturer", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_iProduct,
        { "iProduct", "usb.iProduct", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_iSerialNumber,
        { "iSerialNumber", "usb.iSerialNumber", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bNumConfigurations,
        { "bNumConfigurations", "usb.bNumConfigurations", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_wLANGID,
        { "wLANGID", "usb.wLANGID", FT_UINT16, BASE_HEX, 
          VALS(usb_langid_vals), 0x0, "", HFILL }},

        { &hf_usb_bString,
        { "bString", "usb.bString", FT_STRING, BASE_NONE, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bInterfaceNumber,
        { "bInterfaceNumber", "usb.bInterfaceNumber", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bAlternateSetting,
        { "bAlternateSetting","usb.bAlternateSetting", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bNumEndpoints,
        { "bNumEndpoints","usb.bNumEndpoints", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bInterfaceClass,
        { "bInterfaceClass", "usb.bInterfaceClass", FT_UINT8, BASE_HEX, 
          VALS(usb_interfaceclass_vals), 0x0, "", HFILL }},

        { &hf_usb_bInterfaceSubClass,
        { "bInterfaceSubClass", "usb.bInterfaceSubClass", FT_UINT8, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bInterfaceProtocol,
        { "bInterfaceProtocol", "usb.bInterfaceProtocol", FT_UINT8, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_iInterface,
        { "iInterface", "usb.iInterface", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bEndpointAddress,
        { "bEndpointAddress", "usb.bEndpointAddress", FT_UINT8, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_configuration_bmAttributes,
        { "Configuration bmAttributes", "usb.configuration.bmAttributes", FT_UINT8, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bmAttributes,
        { "bmAttributes", "usb.bmAttributes", FT_UINT8, BASE_HEX, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bEndpointAttributeTransfer,
        { "Transfertype", "usb.bmAttributes.transfer", FT_UINT8, BASE_HEX, 
          VALS(usb_bmAttributes_transfer_vals), 0x03, "", HFILL }},

        { &hf_usb_bEndpointAttributeSynchonisation,
        { "Synchronisationtype", "usb.bmAttributes.sync", FT_UINT8, BASE_HEX, 
          VALS(usb_bmAttributes_sync_vals), 0x0c, "", HFILL }},

        { &hf_usb_bEndpointAttributeBehaviour,
        { "Behaviourtype", "usb.bmAttributes.behaviour", FT_UINT8, BASE_HEX, 
          VALS(usb_bmAttributes_behaviour_vals), 0x30, "", HFILL }},

        { &hf_usb_wMaxPacketSize,
        { "wMaxPacketSize", "usb.wMaxPacketSize", FT_UINT16, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bInterval,
        { "bInterval", "usb.bInterval", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_wTotalLength,
        { "wTotalLength", "usb.wTotalLength", FT_UINT16, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bNumInterfaces,
        { "bNumInterfaces", "usb.bNumInterfaces", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bConfigurationValue,
        { "bConfigurationValue", "usb.bConfigurationValue", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_iConfiguration,
        { "iConfiguration", "usb.iConfiguration", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_bMaxPower,
        { "bMaxPower", "usb.bMaxPower", FT_UINT8, BASE_DEC, 
          NULL, 0x0, "", HFILL }},

        { &hf_usb_configuration_legacy10buspowered,
        { "Must be 1", "usb.configuration.legacy10buspowered", FT_BOOLEAN, 8, 
          TFS(&tfs_mustbeone), 0x80, "Legacy USB 1.0 bus powered", HFILL }},

        { &hf_usb_configuration_selfpowered,
        { "Self-Powered", "usb.configuration.selfpowered", FT_BOOLEAN, 8, 
          TFS(&tfs_selfpowered), 0x40, "", HFILL }},

        { &hf_usb_configuration_remotewakeup,
        { "Remote Wakeup", "usb.configuration.remotewakeup", FT_BOOLEAN, 8, 
          TFS(&tfs_remotewakeup), 0x20, "", HFILL }},

        { &hf_usb_bEndpointAddress_number,
        { "Endpoint Number", "usb.bEndpointAddress.number", FT_UINT8, BASE_HEX, 
          NULL, 0x0f, "", HFILL }},

        { &hf_usb_bEndpointAddress_direction,
        { "Direction", "usb.bEndpointAddress.direction", FT_BOOLEAN, 8, 
          TFS(&tfs_endpoint_direction), 0x80, "", HFILL }},

	{ &hf_usb_request_in,
		{ "Request in", "usb.request_in", FT_FRAMENUM, BASE_NONE,
		NULL, 0, "The request to this packet is in this packet", HFILL }},

	{ &hf_usb_time,
		{ "Time from request", "usb.time", FT_RELATIVE_TIME, BASE_NONE,
		NULL, 0, "Time between Request and Response for USB cmds", HFILL }},

	{ &hf_usb_response_in,
		{ "Response in", "usb.response_in", FT_FRAMENUM, BASE_NONE,
		NULL, 0, "The response to this packet is in this packet", HFILL }},

    };
    
    static gint *usb_subtrees[] = {
            &usb_hdr,
            &usb_setup_hdr,
            &ett_usb_setup_bmrequesttype,
            &ett_descriptor_device,
            &ett_configuration_bmAttributes,
            &ett_configuration_bEndpointAddress,
            &ett_endpoint_bmAttributes
    };

     
    proto_usb = proto_register_protocol("USB", "USB", "usb");
    proto_register_field_array(proto_usb, hf, array_length(hf));
    proto_register_subtree_array(usb_subtrees, array_length(usb_subtrees));

    usb_bulk_dissector_table = register_dissector_table("usb.bulk",
        "USB bulk endpoint", FT_UINT8, BASE_DEC);

    usb_control_dissector_table = register_dissector_table("usb.control",
        "USB control endpoint", FT_UINT8, BASE_DEC);

    usb_tap=register_tap("usb");
}

void
proto_reg_handoff_usb(void)
{
    dissector_handle_t linux_usb_handle;

    linux_usb_handle = create_dissector_handle(dissect_linux_usb, proto_usb);

    dissector_add("wtap_encap", WTAP_ENCAP_USB_LINUX, linux_usb_handle);
}
