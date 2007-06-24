/* Do not modify this file.                                                   */
/* It is created automatically by the ASN.1 to Wireshark dissector compiler   */
/* ./packet-x420.c                                                            */
/* ../../tools/asn2wrs.py -b -e -p x420 -c x420.cnf -s packet-x420-template x420.asn ftbp.asn */

/* Input file: packet-x420-template.c */

#line 1 "packet-x420-template.c"
/* packet-x420.c
 * Routines for X.420 (X.400 Message Transfer)  packet dissection
 * Graeme Lunt 2005
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
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
#include <epan/conversation.h>
#include <epan/oid_resolv.h>
#include <epan/asn1.h>

#include <stdio.h>
#include <string.h>

#include "packet-ber.h"
#include "packet-acse.h"
#include "packet-ros.h"

#include "packet-x509af.h"
#include "packet-x509ce.h"
#include "packet-ftam.h"
#include "packet-x411.h"

#include "packet-x420.h"

#define PNAME  "X.420 Information Object"
#define PSNAME "X420"
#define PFNAME "x420"

/* Initialize the protocol and registered fields */
int proto_x420 = -1;

static const char *object_identifier_id; /* content type identifier */

static const value_string charsetreg_vals [] = {
  { 1, "C0: (ISO/IEC 6429)"},
  { 6, "G0: ASCII (ISO/IEC 646)"},
  { 77, "C1: (ISO/IEC 6429)"},
  { 100, "Gn: Latin Alphabet No.1, Western European Supplementary Set (GR area of ISO-8859-1)"},
  { 101, "Gn: Latin Alphabet No.2, Central EuropeanSupplementary Set (GR area of ISO-8859-2)"},
  { 104, "C0: (ISO/IEC 4873)"},
  { 105, "C1: (ISO/IEC 4873)"},
  { 106, "C0: Teletex (CCITT T.61)"},
  { 107, "C1: Teletex (CCITT T.61)"},
  { 109, "Gn: Latin Alphabet No.3, Southern European Supplementary Set (GR area of ISO-8859-3)"},
  { 110, "Gn: Latin Alphabet No.4, Baltic Supplementary Set (GR area of ISO-8859-4)"},
  { 126, "Gn: Greek Supplementary Set (GR area of ISO-8859-7)"},
  { 127, "Gn: Arabic Supplementary Set (GR area of ISO-8859-6)"},
  { 138, "Gn: Hebrew Supplementary Set (GR area of ISO-8859-8)"},
  { 144, "Gn: Cyrillic Supplementary Set (GR area of ISO-8859-5)"},
  { 148, "Gn: Latin Alphabet No.5, Cyrillic Supplementary Set (GR area of ISO-8859-9)"},
  { 154, "Gn: Supplementary Set for Latin Alphabets No.1 or No.5, and No.2"},
  { 157, "Gn: Latin Alphabet No.6, Arabic Supplementary Set (GR area of ISO-8859-10)"},
  { 158, "Gn: Supplementary Set for Sami (Lappish) to complement Latin Alphabet No.6 (from Annex A  of ISO-8859-10)"},
  { 166, "Gn: Thai Supplementary Set (GR area of ISO-8859-11)"},
  { 179, "Gn: Latin Alphabet No.7, Baltic Rim Supplementary Set (GR area of ISO-8859-13)"},
  { 182, "Gn: Welsh Variant of Latin Alphabet No.1, Supplementary Set (GR area of ISO-8859-1)"},
  { 197, "Gn: Supplementary Set for Sami to complement Latin Alphabet No.6 (from Annex A  of ISO-8859-10)"},
  { 199, "Gn: Latin Alphabet No.8, Celtic Supplementary Set (GR area of ISO-8859-14)"},
  { 203, "Gn: Latin Alphabet No.9, European Rim Supplementary Set (GR area of ISO-8859-15)"},
  { 0, NULL}
};


/*--- Included file: packet-x420-hf.c ---*/
#line 1 "packet-x420-hf.c"
static int hf_x420_InformationObject_PDU = -1;    /* InformationObject */
static int hf_x420_IA5TextParameters_PDU = -1;    /* IA5TextParameters */
static int hf_x420_IA5TextData_PDU = -1;          /* IA5TextData */
static int hf_x420_G3FacsimileParameters_PDU = -1;  /* G3FacsimileParameters */
static int hf_x420_G3FacsimileData_PDU = -1;      /* G3FacsimileData */
static int hf_x420_G4Class1Data_PDU = -1;         /* G4Class1Data */
static int hf_x420_MixedModeData_PDU = -1;        /* MixedModeData */
static int hf_x420_TeletexParameters_PDU = -1;    /* TeletexParameters */
static int hf_x420_TeletexData_PDU = -1;          /* TeletexData */
static int hf_x420_VideotexParameters_PDU = -1;   /* VideotexParameters */
static int hf_x420_VideotexData_PDU = -1;         /* VideotexData */
static int hf_x420_EncryptedParameters_PDU = -1;  /* EncryptedParameters */
static int hf_x420_EncryptedData_PDU = -1;        /* EncryptedData */
static int hf_x420_MessageParameters_PDU = -1;    /* MessageParameters */
static int hf_x420_MessageData_PDU = -1;          /* MessageData */
static int hf_x420_BilaterallyDefinedBodyPart_PDU = -1;  /* BilaterallyDefinedBodyPart */
static int hf_x420_IPN_PDU = -1;                  /* IPN */
static int hf_x420_AbsenceAdvice_PDU = -1;        /* AbsenceAdvice */
static int hf_x420_ChangeOfAddressAdvice_PDU = -1;  /* ChangeOfAddressAdvice */
static int hf_x420_IPMAssemblyInstructions_PDU = -1;  /* IPMAssemblyInstructions */
static int hf_x420_OriginatingUA_PDU = -1;        /* OriginatingUA */
static int hf_x420_IncompleteCopy_PDU = -1;       /* IncompleteCopy */
static int hf_x420_Languages_PDU = -1;            /* Languages */
static int hf_x420_AutoSubmitted_PDU = -1;        /* AutoSubmitted */
static int hf_x420_BodyPartSignatures_PDU = -1;   /* BodyPartSignatures */
static int hf_x420_IPMSecurityLabel_PDU = -1;     /* IPMSecurityLabel */
static int hf_x420_AuthorizationTime_PDU = -1;    /* AuthorizationTime */
static int hf_x420_CirculationList_PDU = -1;      /* CirculationList */
static int hf_x420_CirculationListIndicator_PDU = -1;  /* CirculationListIndicator */
static int hf_x420_DistributionCodes_PDU = -1;    /* DistributionCodes */
static int hf_x420_ExtendedSubject_PDU = -1;      /* ExtendedSubject */
static int hf_x420_InformationCategories_PDU = -1;  /* InformationCategories */
static int hf_x420_ManualHandlingInstructions_PDU = -1;  /* ManualHandlingInstructions */
static int hf_x420_OriginatorsReference_PDU = -1;  /* OriginatorsReference */
static int hf_x420_PrecedencePolicyIdentifier_PDU = -1;  /* PrecedencePolicyIdentifier */
static int hf_x420_Precedence_PDU = -1;           /* Precedence */
static int hf_x420_GeneralTextParameters_PDU = -1;  /* GeneralTextParameters */
static int hf_x420_GeneralTextData_PDU = -1;      /* GeneralTextData */
static int hf_x420_VoiceParameters_PDU = -1;      /* VoiceParameters */
static int hf_x420_VoiceData_PDU = -1;            /* VoiceData */
static int hf_x420_ForwardedContentParameters_PDU = -1;  /* ForwardedContentParameters */
static int hf_x420_FileTransferParameters_PDU = -1;  /* FileTransferParameters */
static int hf_x420_FileTransferData_PDU = -1;     /* FileTransferData */
static int hf_x420_ipm = -1;                      /* IPM */
static int hf_x420_ipn = -1;                      /* IPN */
static int hf_x420_heading = -1;                  /* Heading */
static int hf_x420_body = -1;                     /* Body */
static int hf_x420_type = -1;                     /* T_type */
static int hf_x420_value = -1;                    /* T_value */
static int hf_x420_this_IPM = -1;                 /* ThisIPMField */
static int hf_x420_originator = -1;               /* OriginatorField */
static int hf_x420_authorizing_users = -1;        /* AuthorizingUsersField */
static int hf_x420_primary_recipients = -1;       /* PrimaryRecipientsField */
static int hf_x420_copy_recipients = -1;          /* CopyRecipientsField */
static int hf_x420_blind_copy_recipients = -1;    /* BlindCopyRecipientsField */
static int hf_x420_replied_to_IPM = -1;           /* RepliedToIPMField */
static int hf_x420_obsoleted_IPMs = -1;           /* ObsoletedIPMsField */
static int hf_x420_related_IPMs = -1;             /* RelatedIPMsField */
static int hf_x420_subject = -1;                  /* SubjectField */
static int hf_x420_expiry_time = -1;              /* ExpiryTimeField */
static int hf_x420_reply_time = -1;               /* ReplyTimeField */
static int hf_x420_reply_recipients = -1;         /* ReplyRecipientsField */
static int hf_x420_importance = -1;               /* ImportanceField */
static int hf_x420_sensitivity = -1;              /* SensitivityField */
static int hf_x420_auto_forwarded = -1;           /* AutoForwardedField */
static int hf_x420_extensions = -1;               /* ExtensionsField */
static int hf_x420_user = -1;                     /* ORName */
static int hf_x420_user_relative_identifier = -1;  /* LocalIPMIdentifier */
static int hf_x420_recipient = -1;                /* ORDescriptor */
static int hf_x420_notification_requests = -1;    /* NotificationRequests */
static int hf_x420_reply_requested = -1;          /* BOOLEAN */
static int hf_x420_recipient_extensions = -1;     /* RecipientExtensionsField */
static int hf_x420_formal_name = -1;              /* ORName */
static int hf_x420_free_form_name = -1;           /* FreeFormName */
static int hf_x420_telephone_number = -1;         /* TelephoneNumber */
static int hf_x420_RecipientExtensionsField_item = -1;  /* IPMSExtension */
static int hf_x420_AuthorizingUsersField_item = -1;  /* AuthorizingUsersSubfield */
static int hf_x420_PrimaryRecipientsField_item = -1;  /* PrimaryRecipientsSubfield */
static int hf_x420_CopyRecipientsField_item = -1;  /* CopyRecipientsSubfield */
static int hf_x420_BlindCopyRecipientsField_item = -1;  /* BlindCopyRecipientsSubfield */
static int hf_x420_ObsoletedIPMsField_item = -1;  /* ObsoletedIPMsSubfield */
static int hf_x420_RelatedIPMsField_item = -1;    /* RelatedIPMsSubfield */
static int hf_x420_ReplyRecipientsField_item = -1;  /* ReplyRecipientsSubfield */
static int hf_x420_ExtensionsField_item = -1;     /* IPMSExtension */
static int hf_x420_Body_item = -1;                /* BodyPart */
static int hf_x420_ia5_text = -1;                 /* IA5TextBodyPart */
static int hf_x420_g3_facsimile = -1;             /* G3FacsimileBodyPart */
static int hf_x420_g4_class1 = -1;                /* G4Class1BodyPart */
static int hf_x420_teletex = -1;                  /* TeletexBodyPart */
static int hf_x420_videotex = -1;                 /* VideotexBodyPart */
static int hf_x420_encrypted_bp = -1;             /* EncryptedBodyPart */
static int hf_x420_message = -1;                  /* MessageBodyPart */
static int hf_x420_mixed_mode = -1;               /* MixedModeBodyPart */
static int hf_x420_bilaterally_defined = -1;      /* BilaterallyDefinedBodyPart */
static int hf_x420_nationally_defined = -1;       /* NationallyDefinedBodyPart */
static int hf_x420_extended = -1;                 /* ExtendedBodyPart */
static int hf_x420_extended_parameters = -1;      /* EXTERNALt */
static int hf_x420_extended_data = -1;            /* EXTERNALt */
static int hf_x420_ia5text_parameters = -1;       /* IA5TextParameters */
static int hf_x420_ia5text_data = -1;             /* IA5TextData */
static int hf_x420_repertoire = -1;               /* Repertoire */
static int hf_x420_g3facsimile_parameters = -1;   /* G3FacsimileParameters */
static int hf_x420_g3facsimile_data = -1;         /* G3FacsimileData */
static int hf_x420_number_of_pages = -1;          /* INTEGER */
static int hf_x420_g3facsimile_non_basic_parameters = -1;  /* G3FacsimileNonBasicParameters */
static int hf_x420_G3FacsimileData_item = -1;     /* BIT_STRING */
static int hf_x420_G4Class1Data_item = -1;        /* Interchange_Data_Element */
static int hf_x420_MixedModeData_item = -1;       /* Interchange_Data_Element */
static int hf_x420_teletex_parameters = -1;       /* TeletexParameters */
static int hf_x420_teletex_data = -1;             /* TeletexData */
static int hf_x420_telex_compatible = -1;         /* BOOLEAN */
static int hf_x420_teletex_non_basic_parameters = -1;  /* TeletexNonBasicParameters */
static int hf_x420_TeletexData_item = -1;         /* TeletexString */
static int hf_x420_videotex_parameters = -1;      /* VideotexParameters */
static int hf_x420_videotex_data = -1;            /* VideotexData */
static int hf_x420_syntax = -1;                   /* VideotexSyntax */
static int hf_x420_encrypted_parameters = -1;     /* EncryptedParameters */
static int hf_x420_encrypted_data = -1;           /* EncryptedData */
static int hf_x420_algorithm_identifier = -1;     /* AlgorithmIdentifier */
static int hf_x420_originator_certificates = -1;  /* ExtendedCertificates */
static int hf_x420_message_parameters = -1;       /* MessageParameters */
static int hf_x420_message_data = -1;             /* MessageData */
static int hf_x420_delivery_time = -1;            /* MessageDeliveryTime */
static int hf_x420_delivery_envelope = -1;        /* OtherMessageDeliveryFields */
static int hf_x420_subject_ipm = -1;              /* SubjectIPMField */
static int hf_x420_ipn_originator = -1;           /* IPNOriginatorField */
static int hf_x420_ipm_intended_recipient = -1;   /* IPMIntendedRecipientField */
static int hf_x420_conversion_eits = -1;          /* ConversionEITsField */
static int hf_x420_notification_extensions = -1;  /* NotificationExtensionsField */
static int hf_x420_choice = -1;                   /* T_choice */
static int hf_x420_non_receipt_fields = -1;       /* NonReceiptFields */
static int hf_x420_receipt_fields = -1;           /* ReceiptFields */
static int hf_x420_other_notification_type_fields = -1;  /* OtherNotificationTypeFields */
static int hf_x420_non_receipt_reason = -1;       /* NonReceiptReasonField */
static int hf_x420_discard_reason = -1;           /* DiscardReasonField */
static int hf_x420_auto_forward_comment = -1;     /* AutoForwardCommentField */
static int hf_x420_returned_ipm = -1;             /* ReturnedIPMField */
static int hf_x420_nrn_extensions = -1;           /* NRNExtensionsField */
static int hf_x420_receipt_time = -1;             /* ReceiptTimeField */
static int hf_x420_acknowledgment_mode = -1;      /* AcknowledgmentModeField */
static int hf_x420_suppl_receipt_info = -1;       /* SupplReceiptInfoField */
static int hf_x420_rn_extensions = -1;            /* RNExtensionsField */
static int hf_x420_NotificationExtensionsField_item = -1;  /* IPMSExtension */
static int hf_x420_NRNExtensionsField_item = -1;  /* IPMSExtension */
static int hf_x420_RNExtensionsField_item = -1;   /* IPMSExtension */
static int hf_x420_OtherNotificationTypeFields_item = -1;  /* IPMSExtension */
static int hf_x420_advice = -1;                   /* BodyPart */
static int hf_x420_next_available = -1;           /* Time */
static int hf_x420_new_address = -1;              /* ORDescriptor */
static int hf_x420_effective_from = -1;           /* Time */
static int hf_x420_assembly_instructions = -1;    /* BodyPartReferences */
static int hf_x420_BodyPartReferences_item = -1;  /* BodyPartReference */
static int hf_x420_stored_entry = -1;             /* SequenceNumber */
static int hf_x420_stored_content = -1;           /* SequenceNumber */
static int hf_x420_submitted_body_part = -1;      /* INTEGER */
static int hf_x420_stored_body_part = -1;         /* T_stored_body_part */
static int hf_x420_message_entry = -1;            /* SequenceNumber */
static int hf_x420_body_part_number = -1;         /* BodyPartNumber */
static int hf_x420_Languages_item = -1;           /* Language */
static int hf_x420_algorithmIdentifier = -1;      /* AlgorithmIdentifier */
static int hf_x420_encrypted = -1;                /* BIT_STRING */
static int hf_x420_BodyPartSignatures_item = -1;  /* BodyPartSignatures_item */
static int hf_x420_body_part_signature = -1;      /* BodyPartSignature */
static int hf_x420_originator_certificate_selector = -1;  /* CertificateAssertion */
static int hf_x420_content_security_label = -1;   /* SecurityLabel */
static int hf_x420_heading_security_label = -1;   /* SecurityLabel */
static int hf_x420_body_part_security_labels = -1;  /* SEQUENCE_OF_BodyPartSecurityLabel */
static int hf_x420_body_part_security_labels_item = -1;  /* BodyPartSecurityLabel */
static int hf_x420_body_part_unlabelled = -1;     /* NULL */
static int hf_x420_body_part_security_label = -1;  /* SecurityLabel */
static int hf_x420_CirculationList_item = -1;     /* CirculationMember */
static int hf_x420_circulation_recipient = -1;    /* RecipientSpecifier */
static int hf_x420_checked = -1;                  /* Checkmark */
static int hf_x420_simple = -1;                   /* NULL */
static int hf_x420_timestamped = -1;              /* CirculationTime */
static int hf_x420_signed = -1;                   /* CirculationSignature */
static int hf_x420_circulation_signature_algorithm_identifier = -1;  /* CirculationSignatureAlgorithmIdentifier */
static int hf_x420_timestamp = -1;                /* CirculationTime */
static int hf_x420_circulation_signature_data = -1;  /* CirculationSignatureData */
static int hf_x420_DistributionCodes_item = -1;   /* DistributionCode */
static int hf_x420_oid_code = -1;                 /* OBJECT_IDENTIFIER */
static int hf_x420_alphanumeric_code = -1;        /* AlphaCode */
static int hf_x420_or_descriptor = -1;            /* ORDescriptor */
static int hf_x420_InformationCategories_item = -1;  /* InformationCategory */
static int hf_x420_reference = -1;                /* OBJECT_IDENTIFIER */
static int hf_x420_description = -1;              /* DescriptionString */
static int hf_x420_ManualHandlingInstructions_item = -1;  /* ManualHandlingInstruction */
static int hf_x420_GeneralTextParameters_item = -1;  /* CharacterSetRegistration */
static int hf_x420_voice_message_duration = -1;   /* INTEGER */
static int hf_x420_voice_encoding_type = -1;      /* OBJECT_IDENTIFIER */
static int hf_x420_supplementary_information = -1;  /* IA5String */
static int hf_x420_mts_identifier = -1;           /* MessageDeliveryIdentifier */
static int hf_x420_submission_proof = -1;         /* SubmissionProof */
static int hf_x420_proof_of_submission = -1;      /* ProofOfSubmission */
static int hf_x420_originating_MTA_certificate = -1;  /* OriginatingMTACertificate */
static int hf_x420_message_submission_envelope = -1;  /* MessageSubmissionEnvelope */
static int hf_x420_related_stored_file = -1;      /* RelatedStoredFile */
static int hf_x420_contents_type = -1;            /* ContentsTypeParameter */
static int hf_x420_environment = -1;              /* EnvironmentParameter */
static int hf_x420_compression = -1;              /* CompressionParameter */
static int hf_x420_file_attributes = -1;          /* FileAttributes */
static int hf_x420_FileTransferData_item = -1;    /* EXTERNALt */
static int hf_x420_RelatedStoredFile_item = -1;   /* RelatedStoredFile_item */
static int hf_x420_file_identifier = -1;          /* FileIdentifier */
static int hf_x420_relationship = -1;             /* Relationship */
static int hf_x420_pathname_and_version = -1;     /* PathnameandVersion */
static int hf_x420_cross_reference = -1;          /* CrossReference */
static int hf_x420_pathname = -1;                 /* Pathname_Attribute */
static int hf_x420_file_version = -1;             /* GraphicString */
static int hf_x420_application_cross_reference = -1;  /* OCTET_STRING */
static int hf_x420_message_reference = -1;        /* MessageReference */
static int hf_x420_body_part_reference = -1;      /* INTEGER */
static int hf_x420_user_relative_identifier_ref = -1;  /* PrintableString */
static int hf_x420_explicit_relationship = -1;    /* ExplicitRelationship */
static int hf_x420_descriptive_relationship = -1;  /* GraphicString */
static int hf_x420_document_type = -1;            /* T_document_type */
static int hf_x420_document_type_name = -1;       /* Document_Type_Name */
static int hf_x420_parameter = -1;                /* T_parameter */
static int hf_x420_constraint_set_and_abstract_syntax = -1;  /* T_constraint_set_and_abstract_syntax */
static int hf_x420_constraint_set_name = -1;      /* Constraint_Set_Name */
static int hf_x420_abstract_syntax_name = -1;     /* Abstract_Syntax_Name */
static int hf_x420_application_reference = -1;    /* GeneralIdentifier */
static int hf_x420_machine = -1;                  /* GeneralIdentifier */
static int hf_x420_operating_system = -1;         /* OBJECT_IDENTIFIER */
static int hf_x420_user_visible_string = -1;      /* T_user_visible_string */
static int hf_x420_user_visible_string_item = -1;  /* GraphicString */
static int hf_x420_registered_identifier = -1;    /* OBJECT_IDENTIFIER */
static int hf_x420_descriptive_identifier = -1;   /* T_descriptive_identifier */
static int hf_x420_descriptive_identifier_item = -1;  /* GraphicString */
static int hf_x420_compression_algorithm_id = -1;  /* OBJECT_IDENTIFIER */
static int hf_x420_compression_algorithm_param = -1;  /* T_compression_algorithm_param */
static int hf_x420_permitted_actions = -1;        /* Permitted_Actions_Attribute */
static int hf_x420_storage_account = -1;          /* Account_Attribute */
static int hf_x420_date_and_time_of_creation = -1;  /* Date_and_Time_Attribute */
static int hf_x420_date_and_time_of_last_modification = -1;  /* Date_and_Time_Attribute */
static int hf_x420_date_and_time_of_last_read_access = -1;  /* Date_and_Time_Attribute */
static int hf_x420_date_and_time_of_last_attribute_modification = -1;  /* Date_and_Time_Attribute */
static int hf_x420_identity_of_creator = -1;      /* User_Identity_Attribute */
static int hf_x420_identity_of_last_modifier = -1;  /* User_Identity_Attribute */
static int hf_x420_identity_of_last_reader = -1;  /* User_Identity_Attribute */
static int hf_x420_identity_of_last_attribute_modifier = -1;  /* User_Identity_Attribute */
static int hf_x420_object_availability = -1;      /* Object_Availability_Attribute */
static int hf_x420_object_size = -1;              /* Object_Size_Attribute */
static int hf_x420_future_object_size = -1;       /* Object_Size_Attribute */
static int hf_x420_access_control = -1;           /* Access_Control_Attribute */
static int hf_x420_legal_qualifications = -1;     /* Legal_Qualification_Attribute */
static int hf_x420_private_use = -1;              /* Private_Use_Attribute */
static int hf_x420_attribute_extensions = -1;     /* Attribute_Extensions */
static int hf_x420_incomplete_pathname = -1;      /* Pathname */
static int hf_x420_complete_pathname = -1;        /* Pathname */
static int hf_x420_no_value_available = -1;       /* NULL */
static int hf_x420_account_actual_values = -1;    /* Account */
static int hf_x420_identity_actual_values = -1;   /* User_Identity */
static int hf_x420_actual_values = -1;            /* SET_OF_Access_Control_Element */
static int hf_x420_actual_values_item = -1;       /* Access_Control_Element */
static int hf_x420_action_list = -1;              /* Access_Request */
static int hf_x420_concurrency_access = -1;       /* Concurrency_Access */
static int hf_x420_identity = -1;                 /* User_Identity */
static int hf_x420_passwords = -1;                /* Access_Passwords */
static int hf_x420_location = -1;                 /* Application_Entity_Title */
static int hf_x420_read_password = -1;            /* Password */
static int hf_x420_insert_password = -1;          /* Password */
static int hf_x420_replace_password = -1;         /* Password */
static int hf_x420_extend_password = -1;          /* Password */
static int hf_x420_erase_password = -1;           /* Password */
static int hf_x420_read_attribute_password = -1;  /* Password */
static int hf_x420_change_attribute_password = -1;  /* Password */
static int hf_x420_delete_password = -1;          /* Password */
static int hf_x420_pass_passwords = -1;           /* Pass_Passwords */
static int hf_x420_link_password = -1;            /* Password */
static int hf_x420_graphic_string = -1;           /* GraphicString */
static int hf_x420_octet_string = -1;             /* OCTET_STRING */
static int hf_x420_Pass_Passwords_item = -1;      /* Password */
static int hf_x420_ap_title = -1;                 /* AP_title */
static int hf_x420_ae_qualifier = -1;             /* AE_qualifier */
/* named bits */
static int hf_x420_NotificationRequests_rn = -1;
static int hf_x420_NotificationRequests_nrn = -1;
static int hf_x420_NotificationRequests_ipm_return = -1;
static int hf_x420_NotificationRequests_an_supported = -1;
static int hf_x420_NotificationRequests_suppress_an = -1;
static int hf_x420_Access_Request_read = -1;
static int hf_x420_Access_Request_insert = -1;
static int hf_x420_Access_Request_replace = -1;
static int hf_x420_Access_Request_extend = -1;
static int hf_x420_Access_Request_erase = -1;
static int hf_x420_Access_Request_read_attribute = -1;
static int hf_x420_Access_Request_change_attribute = -1;
static int hf_x420_Access_Request_delete_object = -1;

/*--- End of included file: packet-x420-hf.c ---*/
#line 89 "packet-x420-template.c"

/* Initialize the subtree pointers */
static gint ett_x420 = -1;

/*--- Included file: packet-x420-ett.c ---*/
#line 1 "packet-x420-ett.c"
static gint ett_x420_InformationObject = -1;
static gint ett_x420_IPM = -1;
static gint ett_x420_IPMSExtension = -1;
static gint ett_x420_Heading = -1;
static gint ett_x420_IPMIdentifier = -1;
static gint ett_x420_RecipientSpecifier = -1;
static gint ett_x420_ORDescriptor = -1;
static gint ett_x420_NotificationRequests = -1;
static gint ett_x420_RecipientExtensionsField = -1;
static gint ett_x420_AuthorizingUsersField = -1;
static gint ett_x420_PrimaryRecipientsField = -1;
static gint ett_x420_CopyRecipientsField = -1;
static gint ett_x420_BlindCopyRecipientsField = -1;
static gint ett_x420_ObsoletedIPMsField = -1;
static gint ett_x420_RelatedIPMsField = -1;
static gint ett_x420_ReplyRecipientsField = -1;
static gint ett_x420_ExtensionsField = -1;
static gint ett_x420_Body = -1;
static gint ett_x420_BodyPart = -1;
static gint ett_x420_ExtendedBodyPart = -1;
static gint ett_x420_IA5TextBodyPart = -1;
static gint ett_x420_IA5TextParameters = -1;
static gint ett_x420_G3FacsimileBodyPart = -1;
static gint ett_x420_G3FacsimileParameters = -1;
static gint ett_x420_G3FacsimileData = -1;
static gint ett_x420_G4Class1Data = -1;
static gint ett_x420_MixedModeData = -1;
static gint ett_x420_TeletexBodyPart = -1;
static gint ett_x420_TeletexParameters = -1;
static gint ett_x420_TeletexData = -1;
static gint ett_x420_VideotexBodyPart = -1;
static gint ett_x420_VideotexParameters = -1;
static gint ett_x420_EncryptedBodyPart = -1;
static gint ett_x420_EncryptedParameters = -1;
static gint ett_x420_MessageBodyPart = -1;
static gint ett_x420_MessageParameters = -1;
static gint ett_x420_IPN = -1;
static gint ett_x420_T_choice = -1;
static gint ett_x420_NonReceiptFields = -1;
static gint ett_x420_ReceiptFields = -1;
static gint ett_x420_NotificationExtensionsField = -1;
static gint ett_x420_NRNExtensionsField = -1;
static gint ett_x420_RNExtensionsField = -1;
static gint ett_x420_OtherNotificationTypeFields = -1;
static gint ett_x420_AbsenceAdvice = -1;
static gint ett_x420_ChangeOfAddressAdvice = -1;
static gint ett_x420_IPMAssemblyInstructions = -1;
static gint ett_x420_BodyPartReferences = -1;
static gint ett_x420_BodyPartReference = -1;
static gint ett_x420_T_stored_body_part = -1;
static gint ett_x420_Languages = -1;
static gint ett_x420_Signature = -1;
static gint ett_x420_BodyPartSignatures = -1;
static gint ett_x420_BodyPartSignatures_item = -1;
static gint ett_x420_IPMSecurityLabel = -1;
static gint ett_x420_SEQUENCE_OF_BodyPartSecurityLabel = -1;
static gint ett_x420_BodyPartSecurityLabel = -1;
static gint ett_x420_CirculationList = -1;
static gint ett_x420_CirculationMember = -1;
static gint ett_x420_Checkmark = -1;
static gint ett_x420_CirculationSignatureData = -1;
static gint ett_x420_CirculationSignature = -1;
static gint ett_x420_DistributionCodes = -1;
static gint ett_x420_DistributionCode = -1;
static gint ett_x420_InformationCategories = -1;
static gint ett_x420_InformationCategory = -1;
static gint ett_x420_ManualHandlingInstructions = -1;
static gint ett_x420_GeneralTextParameters = -1;
static gint ett_x420_VoiceParameters = -1;
static gint ett_x420_ForwardedContentParameters = -1;
static gint ett_x420_SubmissionProof = -1;
static gint ett_x420_FileTransferParameters = -1;
static gint ett_x420_FileTransferData = -1;
static gint ett_x420_RelatedStoredFile = -1;
static gint ett_x420_RelatedStoredFile_item = -1;
static gint ett_x420_FileIdentifier = -1;
static gint ett_x420_PathnameandVersion = -1;
static gint ett_x420_CrossReference = -1;
static gint ett_x420_MessageReference = -1;
static gint ett_x420_Relationship = -1;
static gint ett_x420_Contents_Type_Attribute = -1;
static gint ett_x420_T_document_type = -1;
static gint ett_x420_T_constraint_set_and_abstract_syntax = -1;
static gint ett_x420_EnvironmentParameter = -1;
static gint ett_x420_T_user_visible_string = -1;
static gint ett_x420_GeneralIdentifier = -1;
static gint ett_x420_T_descriptive_identifier = -1;
static gint ett_x420_CompressionParameter = -1;
static gint ett_x420_FileAttributes = -1;
static gint ett_x420_Pathname_Attribute = -1;
static gint ett_x420_Account_Attribute = -1;
static gint ett_x420_User_Identity_Attribute = -1;
static gint ett_x420_Access_Control_Attribute = -1;
static gint ett_x420_SET_OF_Access_Control_Element = -1;
static gint ett_x420_Access_Control_Element = -1;
static gint ett_x420_Access_Request = -1;
static gint ett_x420_Access_Passwords = -1;
static gint ett_x420_Password = -1;
static gint ett_x420_Pass_Passwords = -1;
static gint ett_x420_Application_Entity_Title = -1;

/*--- End of included file: packet-x420-ett.c ---*/
#line 93 "packet-x420-template.c"


/*--- Included file: packet-x420-fn.c ---*/
#line 1 "packet-x420-fn.c"
/*--- Cyclic dependencies ---*/

/* IPM -> Body -> BodyPart -> MessageBodyPart -> MessageData -> IPM */
int dissect_x420_IPM(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_);

static int dissect_ipm_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IPM(TRUE, tvb, offset, actx, tree, hf_x420_ipm);
}


/*--- Fields for imported types ---*/

static int dissect_user(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_ORName(FALSE, tvb, offset, actx, tree, hf_x420_user);
}
static int dissect_user_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_ORName(TRUE, tvb, offset, actx, tree, hf_x420_user);
}
static int dissect_formal_name(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_ORName(FALSE, tvb, offset, actx, tree, hf_x420_formal_name);
}
static int dissect_extended_parameters_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_acse_EXTERNALt(TRUE, tvb, offset, actx, tree, hf_x420_extended_parameters);
}
static int dissect_extended_data(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_acse_EXTERNALt(FALSE, tvb, offset, actx, tree, hf_x420_extended_data);
}
static int dissect_g3facsimile_non_basic_parameters_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_G3FacsimileNonBasicParameters(TRUE, tvb, offset, actx, tree, hf_x420_g3facsimile_non_basic_parameters);
}
static int dissect_teletex_non_basic_parameters_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_TeletexNonBasicParameters(TRUE, tvb, offset, actx, tree, hf_x420_teletex_non_basic_parameters);
}
static int dissect_algorithm_identifier(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_AlgorithmIdentifier(FALSE, tvb, offset, actx, tree, hf_x420_algorithm_identifier);
}
static int dissect_originator_certificates(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_ExtendedCertificates(FALSE, tvb, offset, actx, tree, hf_x420_originator_certificates);
}
static int dissect_originator_certificates_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_ExtendedCertificates(TRUE, tvb, offset, actx, tree, hf_x420_originator_certificates);
}
static int dissect_delivery_time_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_MessageDeliveryTime(TRUE, tvb, offset, actx, tree, hf_x420_delivery_time);
}
static int dissect_delivery_envelope_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_OtherMessageDeliveryFields(TRUE, tvb, offset, actx, tree, hf_x420_delivery_envelope);
}
static int dissect_algorithmIdentifier(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509af_AlgorithmIdentifier(FALSE, tvb, offset, actx, tree, hf_x420_algorithmIdentifier);
}
static int dissect_originator_certificate_selector_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x509ce_CertificateAssertion(TRUE, tvb, offset, actx, tree, hf_x420_originator_certificate_selector);
}
static int dissect_content_security_label_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_SecurityLabel(TRUE, tvb, offset, actx, tree, hf_x420_content_security_label);
}
static int dissect_heading_security_label_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_SecurityLabel(TRUE, tvb, offset, actx, tree, hf_x420_heading_security_label);
}
static int dissect_body_part_security_label_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_SecurityLabel(TRUE, tvb, offset, actx, tree, hf_x420_body_part_security_label);
}
static int dissect_mts_identifier_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_MessageDeliveryIdentifier(TRUE, tvb, offset, actx, tree, hf_x420_mts_identifier);
}
static int dissect_proof_of_submission_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_ProofOfSubmission(TRUE, tvb, offset, actx, tree, hf_x420_proof_of_submission);
}
static int dissect_originating_MTA_certificate_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_OriginatingMTACertificate(TRUE, tvb, offset, actx, tree, hf_x420_originating_MTA_certificate);
}
static int dissect_message_submission_envelope(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x411_MessageSubmissionEnvelope(FALSE, tvb, offset, actx, tree, hf_x420_message_submission_envelope);
}
static int dissect_FileTransferData_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_acse_EXTERNALt(FALSE, tvb, offset, actx, tree, hf_x420_FileTransferData_item);
}
static int dissect_permitted_actions_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Permitted_Actions_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_permitted_actions);
}
static int dissect_date_and_time_of_creation_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Date_and_Time_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_date_and_time_of_creation);
}
static int dissect_date_and_time_of_last_modification_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Date_and_Time_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_date_and_time_of_last_modification);
}
static int dissect_date_and_time_of_last_read_access_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Date_and_Time_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_date_and_time_of_last_read_access);
}
static int dissect_date_and_time_of_last_attribute_modification_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Date_and_Time_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_date_and_time_of_last_attribute_modification);
}
static int dissect_object_availability_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Object_Availability_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_object_availability);
}
static int dissect_object_size_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Object_Size_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_object_size);
}
static int dissect_future_object_size_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Object_Size_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_future_object_size);
}
static int dissect_legal_qualifications_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Legal_Qualification_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_legal_qualifications);
}
static int dissect_private_use_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Private_Use_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_private_use);
}
static int dissect_attribute_extensions_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Attribute_Extensions(TRUE, tvb, offset, actx, tree, hf_x420_attribute_extensions);
}
static int dissect_incomplete_pathname_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Pathname(TRUE, tvb, offset, actx, tree, hf_x420_incomplete_pathname);
}
static int dissect_complete_pathname_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Pathname(TRUE, tvb, offset, actx, tree, hf_x420_complete_pathname);
}
static int dissect_concurrency_access_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_ftam_Concurrency_Access(TRUE, tvb, offset, actx, tree, hf_x420_concurrency_access);
}
static int dissect_ap_title(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_acse_AP_title(FALSE, tvb, offset, actx, tree, hf_x420_ap_title);
}
static int dissect_ae_qualifier(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_acse_AE_qualifier(FALSE, tvb, offset, actx, tree, hf_x420_ae_qualifier);
}



static int
dissect_x420_Time(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_UTCTime,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_next_available(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Time(FALSE, tvb, offset, actx, tree, hf_x420_next_available);
}
static int dissect_effective_from_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Time(TRUE, tvb, offset, actx, tree, hf_x420_effective_from);
}



static int
dissect_x420_LocalIPMIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_PrintableString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_user_relative_identifier(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_LocalIPMIdentifier(FALSE, tvb, offset, actx, tree, hf_x420_user_relative_identifier);
}


static const ber_old_sequence_t IPMIdentifier_set[] = {
  { BER_CLASS_APP, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_user },
  { BER_CLASS_UNI, BER_UNI_TAG_PrintableString, BER_FLAGS_NOOWNTAG, dissect_user_relative_identifier },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_IPMIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  IPMIdentifier_set, hf_index, ett_x420_IPMIdentifier);

  return offset;
}



static int
dissect_x420_ThisIPMField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_IPMIdentifier(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_this_IPM(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ThisIPMField(FALSE, tvb, offset, actx, tree, hf_x420_this_IPM);
}



static int
dissect_x420_FreeFormName(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_TeletexString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_free_form_name_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_FreeFormName(TRUE, tvb, offset, actx, tree, hf_x420_free_form_name);
}



static int
dissect_x420_TelephoneNumber(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_PrintableString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_telephone_number_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_TelephoneNumber(TRUE, tvb, offset, actx, tree, hf_x420_telephone_number);
}


static const ber_old_sequence_t ORDescriptor_set[] = {
  { BER_CLASS_APP, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_formal_name },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_free_form_name_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_telephone_number_impl },
  { 0, 0, 0, NULL }
};

int
dissect_x420_ORDescriptor(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  ORDescriptor_set, hf_index, ett_x420_ORDescriptor);

  return offset;
}
static int dissect_recipient_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ORDescriptor(TRUE, tvb, offset, actx, tree, hf_x420_recipient);
}
static int dissect_new_address_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ORDescriptor(TRUE, tvb, offset, actx, tree, hf_x420_new_address);
}
static int dissect_or_descriptor_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ORDescriptor(TRUE, tvb, offset, actx, tree, hf_x420_or_descriptor);
}



static int
dissect_x420_OriginatorField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_ORDescriptor(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_originator_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_OriginatorField(TRUE, tvb, offset, actx, tree, hf_x420_originator);
}



static int
dissect_x420_AuthorizingUsersSubfield(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_ORDescriptor(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_AuthorizingUsersField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_AuthorizingUsersSubfield(FALSE, tvb, offset, actx, tree, hf_x420_AuthorizingUsersField_item);
}


static const ber_old_sequence_t AuthorizingUsersField_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_AuthorizingUsersField_item },
};

static int
dissect_x420_AuthorizingUsersField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          AuthorizingUsersField_sequence_of, hf_index, ett_x420_AuthorizingUsersField);

  return offset;
}
static int dissect_authorizing_users_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_AuthorizingUsersField(TRUE, tvb, offset, actx, tree, hf_x420_authorizing_users);
}


static const asn_namedbit NotificationRequests_bits[] = {
  {  0, &hf_x420_NotificationRequests_rn, -1, -1, "rn", NULL },
  {  1, &hf_x420_NotificationRequests_nrn, -1, -1, "nrn", NULL },
  {  2, &hf_x420_NotificationRequests_ipm_return, -1, -1, "ipm-return", NULL },
  {  3, &hf_x420_NotificationRequests_an_supported, -1, -1, "an-supported", NULL },
  {  4, &hf_x420_NotificationRequests_suppress_an, -1, -1, "suppress-an", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_x420_NotificationRequests(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    NotificationRequests_bits, hf_index, ett_x420_NotificationRequests,
                                    NULL);

  return offset;
}
static int dissect_notification_requests_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_NotificationRequests(TRUE, tvb, offset, actx, tree, hf_x420_notification_requests);
}



static int
dissect_x420_BOOLEAN(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_boolean(implicit_tag, actx, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_reply_requested_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BOOLEAN(TRUE, tvb, offset, actx, tree, hf_x420_reply_requested);
}
static int dissect_telex_compatible_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BOOLEAN(TRUE, tvb, offset, actx, tree, hf_x420_telex_compatible);
}



static int
dissect_x420_T_type(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 129 "x420.cnf"
  const char *name = NULL;

    offset = dissect_ber_object_identifier_str(implicit_tag, actx, tree, tvb, offset, hf_index, &object_identifier_id);

  
  name = get_oid_str_name(object_identifier_id);
  proto_item_append_text(tree, " (%s)", name ? name : object_identifier_id); 



  return offset;
}
static int dissect_type(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_T_type(FALSE, tvb, offset, actx, tree, hf_x420_type);
}



static int
dissect_x420_T_value(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 137 "x420.cnf"

  offset=call_ber_oid_callback(object_identifier_id, tvb, offset, actx->pinfo, tree);



  return offset;
}
static int dissect_value(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_T_value(FALSE, tvb, offset, actx, tree, hf_x420_value);
}


static const ber_old_sequence_t IPMSExtension_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_type },
  { BER_CLASS_ANY, 0, BER_FLAGS_NOOWNTAG, dissect_value },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_IPMSExtension(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       IPMSExtension_sequence, hf_index, ett_x420_IPMSExtension);

  return offset;
}
static int dissect_RecipientExtensionsField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IPMSExtension(FALSE, tvb, offset, actx, tree, hf_x420_RecipientExtensionsField_item);
}
static int dissect_ExtensionsField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IPMSExtension(FALSE, tvb, offset, actx, tree, hf_x420_ExtensionsField_item);
}
static int dissect_NotificationExtensionsField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IPMSExtension(FALSE, tvb, offset, actx, tree, hf_x420_NotificationExtensionsField_item);
}
static int dissect_NRNExtensionsField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IPMSExtension(FALSE, tvb, offset, actx, tree, hf_x420_NRNExtensionsField_item);
}
static int dissect_RNExtensionsField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IPMSExtension(FALSE, tvb, offset, actx, tree, hf_x420_RNExtensionsField_item);
}
static int dissect_OtherNotificationTypeFields_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IPMSExtension(FALSE, tvb, offset, actx, tree, hf_x420_OtherNotificationTypeFields_item);
}


static const ber_old_sequence_t RecipientExtensionsField_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_RecipientExtensionsField_item },
};

static int
dissect_x420_RecipientExtensionsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     RecipientExtensionsField_set_of, hf_index, ett_x420_RecipientExtensionsField);

  return offset;
}
static int dissect_recipient_extensions_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_RecipientExtensionsField(TRUE, tvb, offset, actx, tree, hf_x420_recipient_extensions);
}


static const ber_old_sequence_t RecipientSpecifier_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_recipient_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_notification_requests_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_reply_requested_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_recipient_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_RecipientSpecifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  RecipientSpecifier_set, hf_index, ett_x420_RecipientSpecifier);

  return offset;
}
static int dissect_circulation_recipient(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_RecipientSpecifier(FALSE, tvb, offset, actx, tree, hf_x420_circulation_recipient);
}



static int
dissect_x420_PrimaryRecipientsSubfield(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_RecipientSpecifier(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_PrimaryRecipientsField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_PrimaryRecipientsSubfield(FALSE, tvb, offset, actx, tree, hf_x420_PrimaryRecipientsField_item);
}


static const ber_old_sequence_t PrimaryRecipientsField_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_PrimaryRecipientsField_item },
};

static int
dissect_x420_PrimaryRecipientsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          PrimaryRecipientsField_sequence_of, hf_index, ett_x420_PrimaryRecipientsField);

  return offset;
}
static int dissect_primary_recipients_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_PrimaryRecipientsField(TRUE, tvb, offset, actx, tree, hf_x420_primary_recipients);
}



static int
dissect_x420_CopyRecipientsSubfield(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_RecipientSpecifier(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_CopyRecipientsField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_CopyRecipientsSubfield(FALSE, tvb, offset, actx, tree, hf_x420_CopyRecipientsField_item);
}


static const ber_old_sequence_t CopyRecipientsField_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_CopyRecipientsField_item },
};

static int
dissect_x420_CopyRecipientsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          CopyRecipientsField_sequence_of, hf_index, ett_x420_CopyRecipientsField);

  return offset;
}
static int dissect_copy_recipients_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_CopyRecipientsField(TRUE, tvb, offset, actx, tree, hf_x420_copy_recipients);
}



static int
dissect_x420_BlindCopyRecipientsSubfield(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_RecipientSpecifier(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_BlindCopyRecipientsField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BlindCopyRecipientsSubfield(FALSE, tvb, offset, actx, tree, hf_x420_BlindCopyRecipientsField_item);
}


static const ber_old_sequence_t BlindCopyRecipientsField_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_BlindCopyRecipientsField_item },
};

static int
dissect_x420_BlindCopyRecipientsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          BlindCopyRecipientsField_sequence_of, hf_index, ett_x420_BlindCopyRecipientsField);

  return offset;
}
static int dissect_blind_copy_recipients_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BlindCopyRecipientsField(TRUE, tvb, offset, actx, tree, hf_x420_blind_copy_recipients);
}



static int
dissect_x420_RepliedToIPMField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_IPMIdentifier(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_replied_to_IPM_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_RepliedToIPMField(TRUE, tvb, offset, actx, tree, hf_x420_replied_to_IPM);
}



static int
dissect_x420_ObsoletedIPMsSubfield(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_IPMIdentifier(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_ObsoletedIPMsField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ObsoletedIPMsSubfield(FALSE, tvb, offset, actx, tree, hf_x420_ObsoletedIPMsField_item);
}


static const ber_old_sequence_t ObsoletedIPMsField_sequence_of[1] = {
  { BER_CLASS_APP, 11, BER_FLAGS_NOOWNTAG, dissect_ObsoletedIPMsField_item },
};

static int
dissect_x420_ObsoletedIPMsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          ObsoletedIPMsField_sequence_of, hf_index, ett_x420_ObsoletedIPMsField);

  return offset;
}
static int dissect_obsoleted_IPMs_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ObsoletedIPMsField(TRUE, tvb, offset, actx, tree, hf_x420_obsoleted_IPMs);
}



static int
dissect_x420_RelatedIPMsSubfield(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_IPMIdentifier(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_RelatedIPMsField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_RelatedIPMsSubfield(FALSE, tvb, offset, actx, tree, hf_x420_RelatedIPMsField_item);
}


static const ber_old_sequence_t RelatedIPMsField_sequence_of[1] = {
  { BER_CLASS_APP, 11, BER_FLAGS_NOOWNTAG, dissect_RelatedIPMsField_item },
};

static int
dissect_x420_RelatedIPMsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          RelatedIPMsField_sequence_of, hf_index, ett_x420_RelatedIPMsField);

  return offset;
}
static int dissect_related_IPMs_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_RelatedIPMsField(TRUE, tvb, offset, actx, tree, hf_x420_related_IPMs);
}



static int
dissect_x420_SubjectField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 159 "x420.cnf"
  tvbuff_t *subject=NULL;

    offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_TeletexString,
                                            actx, tree, tvb, offset, hf_index,
                                            &subject);


  if(subject && check_col(actx->pinfo->cinfo, COL_INFO))
   col_append_fstr(actx->pinfo->cinfo, COL_INFO, " (%s)", tvb_format_text(subject, 0, tvb_length(subject)));



  return offset;
}
static int dissect_subject(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_SubjectField(FALSE, tvb, offset, actx, tree, hf_x420_subject);
}



static int
dissect_x420_ExpiryTimeField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_Time(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_expiry_time_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ExpiryTimeField(TRUE, tvb, offset, actx, tree, hf_x420_expiry_time);
}



static int
dissect_x420_ReplyTimeField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_Time(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_reply_time_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ReplyTimeField(TRUE, tvb, offset, actx, tree, hf_x420_reply_time);
}



static int
dissect_x420_ReplyRecipientsSubfield(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_ORDescriptor(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_ReplyRecipientsField_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ReplyRecipientsSubfield(FALSE, tvb, offset, actx, tree, hf_x420_ReplyRecipientsField_item);
}


static const ber_old_sequence_t ReplyRecipientsField_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_ReplyRecipientsField_item },
};

static int
dissect_x420_ReplyRecipientsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          ReplyRecipientsField_sequence_of, hf_index, ett_x420_ReplyRecipientsField);

  return offset;
}
static int dissect_reply_recipients_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ReplyRecipientsField(TRUE, tvb, offset, actx, tree, hf_x420_reply_recipients);
}


static const value_string x420_ImportanceField_vals[] = {
  {   0, "low" },
  {   1, "normal" },
  {   2, "high" },
  { 0, NULL }
};


static int
dissect_x420_ImportanceField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_importance_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ImportanceField(TRUE, tvb, offset, actx, tree, hf_x420_importance);
}


static const value_string x420_SensitivityField_vals[] = {
  {   1, "personal" },
  {   2, "private" },
  {   3, "company-confidential" },
  { 0, NULL }
};


static int
dissect_x420_SensitivityField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_sensitivity_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_SensitivityField(TRUE, tvb, offset, actx, tree, hf_x420_sensitivity);
}



static int
dissect_x420_AutoForwardedField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_boolean(implicit_tag, actx, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_auto_forwarded_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_AutoForwardedField(TRUE, tvb, offset, actx, tree, hf_x420_auto_forwarded);
}


static const ber_old_sequence_t ExtensionsField_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_ExtensionsField_item },
};

int
dissect_x420_ExtensionsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     ExtensionsField_set_of, hf_index, ett_x420_ExtensionsField);

  return offset;
}
static int dissect_extensions_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ExtensionsField(TRUE, tvb, offset, actx, tree, hf_x420_extensions);
}


static const ber_old_sequence_t Heading_set[] = {
  { BER_CLASS_APP, 11, BER_FLAGS_NOOWNTAG, dissect_this_IPM },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_originator_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_authorizing_users_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_primary_recipients_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_copy_recipients_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_blind_copy_recipients_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_replied_to_IPM_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_obsoleted_IPMs_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_related_IPMs_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL, dissect_subject },
  { BER_CLASS_CON, 9, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_expiry_time_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_reply_time_impl },
  { BER_CLASS_CON, 11, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_reply_recipients_impl },
  { BER_CLASS_CON, 12, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_importance_impl },
  { BER_CLASS_CON, 13, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_sensitivity_impl },
  { BER_CLASS_CON, 14, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_auto_forwarded_impl },
  { BER_CLASS_CON, 15, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_Heading(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  Heading_set, hf_index, ett_x420_Heading);

  return offset;
}
static int dissect_heading(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Heading(FALSE, tvb, offset, actx, tree, hf_x420_heading);
}


static const value_string x420_Repertoire_vals[] = {
  {   2, "ita2" },
  {   5, "ia5" },
  { 0, NULL }
};


static int
dissect_x420_Repertoire(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_repertoire_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Repertoire(TRUE, tvb, offset, actx, tree, hf_x420_repertoire);
}


static const ber_old_sequence_t IA5TextParameters_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_repertoire_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_IA5TextParameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  IA5TextParameters_set, hf_index, ett_x420_IA5TextParameters);

  return offset;
}
static int dissect_ia5text_parameters(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IA5TextParameters(FALSE, tvb, offset, actx, tree, hf_x420_ia5text_parameters);
}



static int
dissect_x420_IA5TextData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_IA5String,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_ia5text_data(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IA5TextData(FALSE, tvb, offset, actx, tree, hf_x420_ia5text_data);
}


static const ber_old_sequence_t IA5TextBodyPart_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_ia5text_parameters },
  { BER_CLASS_UNI, BER_UNI_TAG_IA5String, BER_FLAGS_NOOWNTAG, dissect_ia5text_data },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_IA5TextBodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       IA5TextBodyPart_sequence, hf_index, ett_x420_IA5TextBodyPart);

  return offset;
}
static int dissect_ia5_text_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IA5TextBodyPart(TRUE, tvb, offset, actx, tree, hf_x420_ia5_text);
}



static int
dissect_x420_INTEGER(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_number_of_pages_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_INTEGER(TRUE, tvb, offset, actx, tree, hf_x420_number_of_pages);
}
static int dissect_submitted_body_part_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_INTEGER(TRUE, tvb, offset, actx, tree, hf_x420_submitted_body_part);
}
static int dissect_voice_message_duration_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_INTEGER(TRUE, tvb, offset, actx, tree, hf_x420_voice_message_duration);
}
static int dissect_body_part_reference_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_INTEGER(TRUE, tvb, offset, actx, tree, hf_x420_body_part_reference);
}


static const ber_old_sequence_t G3FacsimileParameters_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_number_of_pages_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_g3facsimile_non_basic_parameters_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_G3FacsimileParameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  G3FacsimileParameters_set, hf_index, ett_x420_G3FacsimileParameters);

  return offset;
}
static int dissect_g3facsimile_parameters(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_G3FacsimileParameters(FALSE, tvb, offset, actx, tree, hf_x420_g3facsimile_parameters);
}



static int
dissect_x420_BIT_STRING(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    NULL, hf_index, -1,
                                    NULL);

  return offset;
}
static int dissect_G3FacsimileData_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BIT_STRING(FALSE, tvb, offset, actx, tree, hf_x420_G3FacsimileData_item);
}
static int dissect_encrypted(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BIT_STRING(FALSE, tvb, offset, actx, tree, hf_x420_encrypted);
}


static const ber_old_sequence_t G3FacsimileData_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_G3FacsimileData_item },
};

static int
dissect_x420_G3FacsimileData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          G3FacsimileData_sequence_of, hf_index, ett_x420_G3FacsimileData);

  return offset;
}
static int dissect_g3facsimile_data(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_G3FacsimileData(FALSE, tvb, offset, actx, tree, hf_x420_g3facsimile_data);
}


static const ber_old_sequence_t G3FacsimileBodyPart_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_g3facsimile_parameters },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_g3facsimile_data },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_G3FacsimileBodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       G3FacsimileBodyPart_sequence, hf_index, ett_x420_G3FacsimileBodyPart);

  return offset;
}
static int dissect_g3_facsimile_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_G3FacsimileBodyPart(TRUE, tvb, offset, actx, tree, hf_x420_g3_facsimile);
}



static int
dissect_x420_Interchange_Data_Element(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 178 "x420.cnf"
/* XXX Not implemented yet */



  return offset;
}
static int dissect_G4Class1Data_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Interchange_Data_Element(FALSE, tvb, offset, actx, tree, hf_x420_G4Class1Data_item);
}
static int dissect_MixedModeData_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Interchange_Data_Element(FALSE, tvb, offset, actx, tree, hf_x420_MixedModeData_item);
}


static const ber_old_sequence_t G4Class1Data_sequence_of[1] = {
  { BER_CLASS_ANY, 0, BER_FLAGS_NOOWNTAG, dissect_G4Class1Data_item },
};

static int
dissect_x420_G4Class1Data(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          G4Class1Data_sequence_of, hf_index, ett_x420_G4Class1Data);

  return offset;
}



static int
dissect_x420_G4Class1BodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_G4Class1Data(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_g4_class1_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_G4Class1BodyPart(TRUE, tvb, offset, actx, tree, hf_x420_g4_class1);
}


static const ber_old_sequence_t TeletexParameters_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_number_of_pages_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_telex_compatible_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_teletex_non_basic_parameters_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_TeletexParameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  TeletexParameters_set, hf_index, ett_x420_TeletexParameters);

  return offset;
}
static int dissect_teletex_parameters(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_TeletexParameters(FALSE, tvb, offset, actx, tree, hf_x420_teletex_parameters);
}



static int
dissect_x420_TeletexString(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_TeletexString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_TeletexData_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_TeletexString(FALSE, tvb, offset, actx, tree, hf_x420_TeletexData_item);
}


static const ber_old_sequence_t TeletexData_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_TeletexString, BER_FLAGS_NOOWNTAG, dissect_TeletexData_item },
};

static int
dissect_x420_TeletexData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          TeletexData_sequence_of, hf_index, ett_x420_TeletexData);

  return offset;
}
static int dissect_teletex_data(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_TeletexData(FALSE, tvb, offset, actx, tree, hf_x420_teletex_data);
}


static const ber_old_sequence_t TeletexBodyPart_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_teletex_parameters },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_teletex_data },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_TeletexBodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       TeletexBodyPart_sequence, hf_index, ett_x420_TeletexBodyPart);

  return offset;
}
static int dissect_teletex_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_TeletexBodyPart(TRUE, tvb, offset, actx, tree, hf_x420_teletex);
}


static const value_string x420_VideotexSyntax_vals[] = {
  {   0, "ids" },
  {   1, "data-syntax1" },
  {   2, "data-syntax2" },
  {   3, "data-syntax3" },
  { 0, NULL }
};


static int
dissect_x420_VideotexSyntax(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_syntax_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_VideotexSyntax(TRUE, tvb, offset, actx, tree, hf_x420_syntax);
}


static const ber_old_sequence_t VideotexParameters_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_syntax_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_VideotexParameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  VideotexParameters_set, hf_index, ett_x420_VideotexParameters);

  return offset;
}
static int dissect_videotex_parameters(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_VideotexParameters(FALSE, tvb, offset, actx, tree, hf_x420_videotex_parameters);
}



static int
dissect_x420_VideotexData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_VideotexString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_videotex_data(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_VideotexData(FALSE, tvb, offset, actx, tree, hf_x420_videotex_data);
}


static const ber_old_sequence_t VideotexBodyPart_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_videotex_parameters },
  { BER_CLASS_UNI, BER_UNI_TAG_VideotexString, BER_FLAGS_NOOWNTAG, dissect_videotex_data },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_VideotexBodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       VideotexBodyPart_sequence, hf_index, ett_x420_VideotexBodyPart);

  return offset;
}
static int dissect_videotex_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_VideotexBodyPart(TRUE, tvb, offset, actx, tree, hf_x420_videotex);
}


static const ber_old_sequence_t EncryptedParameters_set[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithm_identifier },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_originator_certificates },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_EncryptedParameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  EncryptedParameters_set, hf_index, ett_x420_EncryptedParameters);

  return offset;
}
static int dissect_encrypted_parameters(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_EncryptedParameters(FALSE, tvb, offset, actx, tree, hf_x420_encrypted_parameters);
}



static int
dissect_x420_EncryptedData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    NULL, hf_index, -1,
                                    NULL);

  return offset;
}
static int dissect_encrypted_data(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_EncryptedData(FALSE, tvb, offset, actx, tree, hf_x420_encrypted_data);
}


static const ber_old_sequence_t EncryptedBodyPart_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_encrypted_parameters },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted_data },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_EncryptedBodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       EncryptedBodyPart_sequence, hf_index, ett_x420_EncryptedBodyPart);

  return offset;
}
static int dissect_encrypted_bp_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_EncryptedBodyPart(TRUE, tvb, offset, actx, tree, hf_x420_encrypted_bp);
}


static const ber_old_sequence_t MessageParameters_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_delivery_time_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_delivery_envelope_impl },
  { 0, 0, 0, NULL }
};

int
dissect_x420_MessageParameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  MessageParameters_set, hf_index, ett_x420_MessageParameters);

  return offset;
}
static int dissect_message_parameters(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_MessageParameters(FALSE, tvb, offset, actx, tree, hf_x420_message_parameters);
}



static int
dissect_x420_MessageData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_IPM(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_message_data(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_MessageData(FALSE, tvb, offset, actx, tree, hf_x420_message_data);
}


static const ber_old_sequence_t MessageBodyPart_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_message_parameters },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_message_data },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_MessageBodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       MessageBodyPart_sequence, hf_index, ett_x420_MessageBodyPart);

  return offset;
}
static int dissect_message_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_MessageBodyPart(TRUE, tvb, offset, actx, tree, hf_x420_message);
}


static const ber_old_sequence_t MixedModeData_sequence_of[1] = {
  { BER_CLASS_ANY, 0, BER_FLAGS_NOOWNTAG, dissect_MixedModeData_item },
};

static int
dissect_x420_MixedModeData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          MixedModeData_sequence_of, hf_index, ett_x420_MixedModeData);

  return offset;
}



static int
dissect_x420_MixedModeBodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_MixedModeData(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_mixed_mode_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_MixedModeBodyPart(TRUE, tvb, offset, actx, tree, hf_x420_mixed_mode);
}



static int
dissect_x420_BilaterallyDefinedBodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_bilaterally_defined_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BilaterallyDefinedBodyPart(TRUE, tvb, offset, actx, tree, hf_x420_bilaterally_defined);
}



static int
dissect_x420_NationallyDefinedBodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 181 "x420.cnf"
/* XXX Not implemented yet */



  return offset;
}
static int dissect_nationally_defined_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_NationallyDefinedBodyPart(TRUE, tvb, offset, actx, tree, hf_x420_nationally_defined);
}


static const ber_old_sequence_t ExtendedBodyPart_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_extended_parameters_impl },
  { BER_CLASS_UNI, 8, BER_FLAGS_NOOWNTAG, dissect_extended_data },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_ExtendedBodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       ExtendedBodyPart_sequence, hf_index, ett_x420_ExtendedBodyPart);

  return offset;
}
static int dissect_extended_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ExtendedBodyPart(TRUE, tvb, offset, actx, tree, hf_x420_extended);
}


static const value_string x420_BodyPart_vals[] = {
  {   0, "ia5-text" },
  {   3, "g3-facsimile" },
  {   4, "g4-class1" },
  {   5, "teletex" },
  {   6, "videotex" },
  {   8, "encrypted" },
  {   9, "message" },
  {  11, "mixed-mode" },
  {  14, "bilaterally-defined" },
  {   7, "nationally-defined" },
  {  15, "extended" },
  { 0, NULL }
};

static const ber_old_choice_t BodyPart_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_ia5_text_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_g3_facsimile_impl },
  {   4, BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG, dissect_g4_class1_impl },
  {   5, BER_CLASS_CON, 5, BER_FLAGS_IMPLTAG, dissect_teletex_impl },
  {   6, BER_CLASS_CON, 6, BER_FLAGS_IMPLTAG, dissect_videotex_impl },
  {   8, BER_CLASS_CON, 8, BER_FLAGS_IMPLTAG, dissect_encrypted_bp_impl },
  {   9, BER_CLASS_CON, 9, BER_FLAGS_IMPLTAG, dissect_message_impl },
  {  11, BER_CLASS_CON, 11, BER_FLAGS_IMPLTAG, dissect_mixed_mode_impl },
  {  14, BER_CLASS_CON, 14, BER_FLAGS_IMPLTAG, dissect_bilaterally_defined_impl },
  {   7, BER_CLASS_CON, 7, BER_FLAGS_IMPLTAG, dissect_nationally_defined_impl },
  {  15, BER_CLASS_CON, 15, BER_FLAGS_IMPLTAG, dissect_extended_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_BodyPart(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     BodyPart_choice, hf_index, ett_x420_BodyPart,
                                     NULL);

  return offset;
}
static int dissect_Body_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BodyPart(FALSE, tvb, offset, actx, tree, hf_x420_Body_item);
}
static int dissect_advice(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BodyPart(FALSE, tvb, offset, actx, tree, hf_x420_advice);
}


static const ber_old_sequence_t Body_sequence_of[1] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_Body_item },
};

static int
dissect_x420_Body(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          Body_sequence_of, hf_index, ett_x420_Body);

  return offset;
}
static int dissect_body(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Body(FALSE, tvb, offset, actx, tree, hf_x420_body);
}


static const ber_old_sequence_t IPM_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_heading },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_body },
  { 0, 0, 0, NULL }
};

int
dissect_x420_IPM(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 141 "x420.cnf"

 if(check_col(actx->pinfo->cinfo, COL_INFO))
   col_append_fstr(actx->pinfo->cinfo, COL_INFO, " Message");

    offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       IPM_sequence, hf_index, ett_x420_IPM);





  return offset;
}



static int
dissect_x420_SubjectIPMField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_IPMIdentifier(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_subject_ipm(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_SubjectIPMField(FALSE, tvb, offset, actx, tree, hf_x420_subject_ipm);
}



static int
dissect_x420_IPNOriginatorField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_ORDescriptor(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_ipn_originator_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IPNOriginatorField(TRUE, tvb, offset, actx, tree, hf_x420_ipn_originator);
}



static int
dissect_x420_IPMIntendedRecipientField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_ORDescriptor(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_ipm_intended_recipient_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IPMIntendedRecipientField(TRUE, tvb, offset, actx, tree, hf_x420_ipm_intended_recipient);
}



static int
dissect_x420_ConversionEITsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x411_EncodedInformationTypes(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_conversion_eits(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ConversionEITsField(FALSE, tvb, offset, actx, tree, hf_x420_conversion_eits);
}


static const ber_old_sequence_t NotificationExtensionsField_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_NotificationExtensionsField_item },
};

static int
dissect_x420_NotificationExtensionsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     NotificationExtensionsField_set_of, hf_index, ett_x420_NotificationExtensionsField);

  return offset;
}
static int dissect_notification_extensions_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_NotificationExtensionsField(TRUE, tvb, offset, actx, tree, hf_x420_notification_extensions);
}


const value_string x420_NonReceiptReasonField_vals[] = {
  {   0, "ipm-discarded" },
  {   1, "ipm-auto-forwarded" },
  { 0, NULL }
};


int
dissect_x420_NonReceiptReasonField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_non_receipt_reason_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_NonReceiptReasonField(TRUE, tvb, offset, actx, tree, hf_x420_non_receipt_reason);
}


const value_string x420_DiscardReasonField_vals[] = {
  {   0, "ipm-expired" },
  {   1, "ipm-obsoleted" },
  {   2, "user-subscription-terminated" },
  {   3, "not-used" },
  { 0, NULL }
};


int
dissect_x420_DiscardReasonField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_discard_reason_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_DiscardReasonField(TRUE, tvb, offset, actx, tree, hf_x420_discard_reason);
}



static int
dissect_x420_AutoForwardComment(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_PrintableString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}



static int
dissect_x420_AutoForwardCommentField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_AutoForwardComment(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_auto_forward_comment_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_AutoForwardCommentField(TRUE, tvb, offset, actx, tree, hf_x420_auto_forward_comment);
}



static int
dissect_x420_ReturnedIPMField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_IPM(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_returned_ipm_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ReturnedIPMField(TRUE, tvb, offset, actx, tree, hf_x420_returned_ipm);
}


static const ber_old_sequence_t NRNExtensionsField_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_NRNExtensionsField_item },
};

static int
dissect_x420_NRNExtensionsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     NRNExtensionsField_set_of, hf_index, ett_x420_NRNExtensionsField);

  return offset;
}
static int dissect_nrn_extensions_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_NRNExtensionsField(TRUE, tvb, offset, actx, tree, hf_x420_nrn_extensions);
}


static const ber_old_sequence_t NonReceiptFields_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_non_receipt_reason_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_discard_reason_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_auto_forward_comment_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_returned_ipm_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_nrn_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_NonReceiptFields(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  NonReceiptFields_set, hf_index, ett_x420_NonReceiptFields);

  return offset;
}
static int dissect_non_receipt_fields_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_NonReceiptFields(TRUE, tvb, offset, actx, tree, hf_x420_non_receipt_fields);
}



static int
dissect_x420_ReceiptTimeField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_Time(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_receipt_time_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ReceiptTimeField(TRUE, tvb, offset, actx, tree, hf_x420_receipt_time);
}


static const value_string x420_AcknowledgmentModeField_vals[] = {
  {   0, "manual" },
  {   1, "automatic" },
  { 0, NULL }
};


static int
dissect_x420_AcknowledgmentModeField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_acknowledgment_mode_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_AcknowledgmentModeField(TRUE, tvb, offset, actx, tree, hf_x420_acknowledgment_mode);
}



static int
dissect_x420_SupplReceiptInfoField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x411_SupplementaryInformation(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_suppl_receipt_info_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_SupplReceiptInfoField(TRUE, tvb, offset, actx, tree, hf_x420_suppl_receipt_info);
}


static const ber_old_sequence_t RNExtensionsField_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_RNExtensionsField_item },
};

static int
dissect_x420_RNExtensionsField(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     RNExtensionsField_set_of, hf_index, ett_x420_RNExtensionsField);

  return offset;
}
static int dissect_rn_extensions_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_RNExtensionsField(TRUE, tvb, offset, actx, tree, hf_x420_rn_extensions);
}


static const ber_old_sequence_t ReceiptFields_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_receipt_time_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_acknowledgment_mode_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_suppl_receipt_info_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_rn_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_ReceiptFields(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  ReceiptFields_set, hf_index, ett_x420_ReceiptFields);

  return offset;
}
static int dissect_receipt_fields_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ReceiptFields(TRUE, tvb, offset, actx, tree, hf_x420_receipt_fields);
}


static const ber_old_sequence_t OtherNotificationTypeFields_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_OtherNotificationTypeFields_item },
};

static int
dissect_x420_OtherNotificationTypeFields(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     OtherNotificationTypeFields_set_of, hf_index, ett_x420_OtherNotificationTypeFields);

  return offset;
}
static int dissect_other_notification_type_fields_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_OtherNotificationTypeFields(TRUE, tvb, offset, actx, tree, hf_x420_other_notification_type_fields);
}


static const value_string x420_T_choice_vals[] = {
  {   0, "non-receipt-fields" },
  {   1, "receipt-fields" },
  {   2, "other-notification-type-fields" },
  { 0, NULL }
};

static const ber_old_choice_t T_choice_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_non_receipt_fields_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_receipt_fields_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_other_notification_type_fields_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_T_choice(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     T_choice_choice, hf_index, ett_x420_T_choice,
                                     NULL);

  return offset;
}
static int dissect_choice_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_T_choice(TRUE, tvb, offset, actx, tree, hf_x420_choice);
}


static const ber_old_sequence_t IPN_set[] = {
  { BER_CLASS_APP, 11, BER_FLAGS_NOOWNTAG, dissect_subject_ipm },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_ipn_originator_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_ipm_intended_recipient_impl },
  { BER_CLASS_APP, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_conversion_eits },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_notification_extensions_impl },
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_choice_impl },
  { 0, 0, 0, NULL }
};

int
dissect_x420_IPN(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 149 "x420.cnf"

 if(check_col(actx->pinfo->cinfo, COL_INFO))
   col_append_fstr(actx->pinfo->cinfo, COL_INFO, " Notification");

    offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  IPN_set, hf_index, ett_x420_IPN);




  return offset;
}
static int dissect_ipn_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IPN(TRUE, tvb, offset, actx, tree, hf_x420_ipn);
}


const value_string x420_InformationObject_vals[] = {
  {   0, "ipm" },
  {   1, "ipn" },
  { 0, NULL }
};

static const ber_old_choice_t InformationObject_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_ipm_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_ipn_impl },
  { 0, 0, 0, 0, NULL }
};

int
dissect_x420_InformationObject(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     InformationObject_choice, hf_index, ett_x420_InformationObject,
                                     NULL);

  return offset;
}


static const ber_old_sequence_t AbsenceAdvice_sequence[] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_advice },
  { BER_CLASS_UNI, BER_UNI_TAG_UTCTime, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_next_available },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_AbsenceAdvice(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       AbsenceAdvice_sequence, hf_index, ett_x420_AbsenceAdvice);

  return offset;
}


static const ber_old_sequence_t ChangeOfAddressAdvice_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_new_address_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_effective_from_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_ChangeOfAddressAdvice(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       ChangeOfAddressAdvice_sequence, hf_index, ett_x420_ChangeOfAddressAdvice);

  return offset;
}



static int
dissect_x420_SequenceNumber(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_stored_entry_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_SequenceNumber(TRUE, tvb, offset, actx, tree, hf_x420_stored_entry);
}
static int dissect_stored_content_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_SequenceNumber(TRUE, tvb, offset, actx, tree, hf_x420_stored_content);
}
static int dissect_message_entry(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_SequenceNumber(FALSE, tvb, offset, actx, tree, hf_x420_message_entry);
}



static int
dissect_x420_BodyPartNumber(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_body_part_number(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BodyPartNumber(FALSE, tvb, offset, actx, tree, hf_x420_body_part_number);
}


static const ber_old_sequence_t T_stored_body_part_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_message_entry },
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_body_part_number },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_T_stored_body_part(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       T_stored_body_part_sequence, hf_index, ett_x420_T_stored_body_part);

  return offset;
}
static int dissect_stored_body_part_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_T_stored_body_part(TRUE, tvb, offset, actx, tree, hf_x420_stored_body_part);
}


static const value_string x420_BodyPartReference_vals[] = {
  {   0, "stored-entry" },
  {   1, "stored-content" },
  {   2, "submitted-body-part" },
  {   3, "stored-body-part" },
  { 0, NULL }
};

static const ber_old_choice_t BodyPartReference_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_stored_entry_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_stored_content_impl },
  {   2, BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG, dissect_submitted_body_part_impl },
  {   3, BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG, dissect_stored_body_part_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_BodyPartReference(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     BodyPartReference_choice, hf_index, ett_x420_BodyPartReference,
                                     NULL);

  return offset;
}
static int dissect_BodyPartReferences_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BodyPartReference(FALSE, tvb, offset, actx, tree, hf_x420_BodyPartReferences_item);
}


static const ber_old_sequence_t BodyPartReferences_sequence_of[1] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_BodyPartReferences_item },
};

static int
dissect_x420_BodyPartReferences(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          BodyPartReferences_sequence_of, hf_index, ett_x420_BodyPartReferences);

  return offset;
}
static int dissect_assembly_instructions_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BodyPartReferences(TRUE, tvb, offset, actx, tree, hf_x420_assembly_instructions);
}


static const ber_old_sequence_t IPMAssemblyInstructions_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_assembly_instructions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_IPMAssemblyInstructions(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  IPMAssemblyInstructions_set, hf_index, ett_x420_IPMAssemblyInstructions);

  return offset;
}



static int
dissect_x420_OriginatingUA(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_IA5String,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}



static int
dissect_x420_IncompleteCopy(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_null(implicit_tag, actx, tree, tvb, offset, hf_index);

  return offset;
}



static int
dissect_x420_Language(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_PrintableString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_Languages_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Language(FALSE, tvb, offset, actx, tree, hf_x420_Languages_item);
}


static const ber_old_sequence_t Languages_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_PrintableString, BER_FLAGS_NOOWNTAG, dissect_Languages_item },
};

static int
dissect_x420_Languages(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     Languages_set_of, hf_index, ett_x420_Languages);

  return offset;
}


static const value_string x420_AutoSubmitted_vals[] = {
  {   0, "not-auto-submitted" },
  {   1, "auto-generated" },
  {   2, "auto-replied" },
  { 0, NULL }
};


static int
dissect_x420_AutoSubmitted(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}


static const ber_old_sequence_t Signature_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithmIdentifier },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_Signature(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Signature_sequence, hf_index, ett_x420_Signature);

  return offset;
}



static int
dissect_x420_BodyPartSignature(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_Signature(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_body_part_signature(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BodyPartSignature(FALSE, tvb, offset, actx, tree, hf_x420_body_part_signature);
}


static const ber_old_sequence_t BodyPartSignatures_item_set[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_body_part_number },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_body_part_signature },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_originator_certificate_selector_impl },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_originator_certificates_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_BodyPartSignatures_item(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  BodyPartSignatures_item_set, hf_index, ett_x420_BodyPartSignatures_item);

  return offset;
}
static int dissect_BodyPartSignatures_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BodyPartSignatures_item(FALSE, tvb, offset, actx, tree, hf_x420_BodyPartSignatures_item);
}


static const ber_old_sequence_t BodyPartSignatures_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_BodyPartSignatures_item },
};

static int
dissect_x420_BodyPartSignatures(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     BodyPartSignatures_set_of, hf_index, ett_x420_BodyPartSignatures);

  return offset;
}



static int
dissect_x420_NULL(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_null(implicit_tag, actx, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_body_part_unlabelled_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_NULL(TRUE, tvb, offset, actx, tree, hf_x420_body_part_unlabelled);
}
static int dissect_simple(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_NULL(FALSE, tvb, offset, actx, tree, hf_x420_simple);
}
static int dissect_no_value_available_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_NULL(TRUE, tvb, offset, actx, tree, hf_x420_no_value_available);
}


static const value_string x420_BodyPartSecurityLabel_vals[] = {
  {   0, "body-part-unlabelled" },
  {   1, "body-part-security-label" },
  { 0, NULL }
};

static const ber_old_choice_t BodyPartSecurityLabel_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_body_part_unlabelled_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_body_part_security_label_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_BodyPartSecurityLabel(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     BodyPartSecurityLabel_choice, hf_index, ett_x420_BodyPartSecurityLabel,
                                     NULL);

  return offset;
}
static int dissect_body_part_security_labels_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_BodyPartSecurityLabel(FALSE, tvb, offset, actx, tree, hf_x420_body_part_security_labels_item);
}


static const ber_old_sequence_t SEQUENCE_OF_BodyPartSecurityLabel_sequence_of[1] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_body_part_security_labels_item },
};

static int
dissect_x420_SEQUENCE_OF_BodyPartSecurityLabel(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          SEQUENCE_OF_BodyPartSecurityLabel_sequence_of, hf_index, ett_x420_SEQUENCE_OF_BodyPartSecurityLabel);

  return offset;
}
static int dissect_body_part_security_labels_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_SEQUENCE_OF_BodyPartSecurityLabel(TRUE, tvb, offset, actx, tree, hf_x420_body_part_security_labels);
}


static const ber_old_sequence_t IPMSecurityLabel_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_content_security_label_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_heading_security_label_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_body_part_security_labels_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_IPMSecurityLabel(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       IPMSecurityLabel_sequence, hf_index, ett_x420_IPMSecurityLabel);

  return offset;
}



static int
dissect_x420_AuthorizationTime(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_GeneralizedTime(implicit_tag, actx, tree, tvb, offset, hf_index);

  return offset;
}



static int
dissect_x420_CirculationTime(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_GeneralizedTime(implicit_tag, actx, tree, tvb, offset, hf_index);

  return offset;
}
static int dissect_timestamped(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_CirculationTime(FALSE, tvb, offset, actx, tree, hf_x420_timestamped);
}
static int dissect_timestamp(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_CirculationTime(FALSE, tvb, offset, actx, tree, hf_x420_timestamp);
}



static int
dissect_x420_CirculationSignatureAlgorithmIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x509af_AlgorithmIdentifier(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_circulation_signature_algorithm_identifier(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_CirculationSignatureAlgorithmIdentifier(FALSE, tvb, offset, actx, tree, hf_x420_circulation_signature_algorithm_identifier);
}


static const ber_old_sequence_t CirculationSignatureData_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_circulation_signature_algorithm_identifier },
  { BER_CLASS_APP, 11, BER_FLAGS_NOOWNTAG, dissect_this_IPM },
  { BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_NOOWNTAG, dissect_timestamp },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_CirculationSignatureData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       CirculationSignatureData_sequence, hf_index, ett_x420_CirculationSignatureData);

  return offset;
}
static int dissect_circulation_signature_data(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_CirculationSignatureData(FALSE, tvb, offset, actx, tree, hf_x420_circulation_signature_data);
}


static const ber_old_sequence_t CirculationSignature_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_circulation_signature_data },
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_algorithm_identifier },
  { BER_CLASS_UNI, BER_UNI_TAG_BITSTRING, BER_FLAGS_NOOWNTAG, dissect_encrypted },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_CirculationSignature(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       CirculationSignature_sequence, hf_index, ett_x420_CirculationSignature);

  return offset;
}
static int dissect_signed(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_CirculationSignature(FALSE, tvb, offset, actx, tree, hf_x420_signed);
}


static const value_string x420_Checkmark_vals[] = {
  {   0, "simple" },
  {   1, "timestamped" },
  {   2, "signed" },
  { 0, NULL }
};

static const ber_old_choice_t Checkmark_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_NULL, BER_FLAGS_NOOWNTAG, dissect_simple },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_GeneralizedTime, BER_FLAGS_NOOWNTAG, dissect_timestamped },
  {   2, BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_signed },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_Checkmark(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     Checkmark_choice, hf_index, ett_x420_Checkmark,
                                     NULL);

  return offset;
}
static int dissect_checked(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Checkmark(FALSE, tvb, offset, actx, tree, hf_x420_checked);
}


static const ber_old_sequence_t CirculationMember_set[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_circulation_recipient },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_checked },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_CirculationMember(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  CirculationMember_set, hf_index, ett_x420_CirculationMember);

  return offset;
}
static int dissect_CirculationList_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_CirculationMember(FALSE, tvb, offset, actx, tree, hf_x420_CirculationList_item);
}


static const ber_old_sequence_t CirculationList_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_CirculationList_item },
};

static int
dissect_x420_CirculationList(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          CirculationList_sequence_of, hf_index, ett_x420_CirculationList);

  return offset;
}



static int
dissect_x420_CirculationListIndicator(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_null(implicit_tag, actx, tree, tvb, offset, hf_index);

  return offset;
}



static int
dissect_x420_OBJECT_IDENTIFIER(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, actx, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_oid_code(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_OBJECT_IDENTIFIER(FALSE, tvb, offset, actx, tree, hf_x420_oid_code);
}
static int dissect_reference_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_OBJECT_IDENTIFIER(TRUE, tvb, offset, actx, tree, hf_x420_reference);
}
static int dissect_voice_encoding_type_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_OBJECT_IDENTIFIER(TRUE, tvb, offset, actx, tree, hf_x420_voice_encoding_type);
}
static int dissect_operating_system_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_OBJECT_IDENTIFIER(TRUE, tvb, offset, actx, tree, hf_x420_operating_system);
}
static int dissect_registered_identifier_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_OBJECT_IDENTIFIER(TRUE, tvb, offset, actx, tree, hf_x420_registered_identifier);
}
static int dissect_compression_algorithm_id_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_OBJECT_IDENTIFIER(TRUE, tvb, offset, actx, tree, hf_x420_compression_algorithm_id);
}



static int
dissect_x420_AlphaCode(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x411_UniversalOrBMPString(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_alphanumeric_code(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_AlphaCode(FALSE, tvb, offset, actx, tree, hf_x420_alphanumeric_code);
}


static const ber_old_sequence_t DistributionCode_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_oid_code },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG, dissect_alphanumeric_code },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_or_descriptor_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_DistributionCode(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       DistributionCode_sequence, hf_index, ett_x420_DistributionCode);

  return offset;
}
static int dissect_DistributionCodes_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_DistributionCode(FALSE, tvb, offset, actx, tree, hf_x420_DistributionCodes_item);
}


static const ber_old_sequence_t DistributionCodes_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_DistributionCodes_item },
};

static int
dissect_x420_DistributionCodes(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          DistributionCodes_sequence_of, hf_index, ett_x420_DistributionCodes);

  return offset;
}



static int
dissect_x420_ExtendedSubject(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x411_UniversalOrBMPString(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}



static int
dissect_x420_DescriptionString(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x411_UniversalOrBMPString(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_description_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_DescriptionString(TRUE, tvb, offset, actx, tree, hf_x420_description);
}


static const ber_old_sequence_t InformationCategory_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_reference_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_description_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_InformationCategory(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       InformationCategory_sequence, hf_index, ett_x420_InformationCategory);

  return offset;
}
static int dissect_InformationCategories_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_InformationCategory(FALSE, tvb, offset, actx, tree, hf_x420_InformationCategories_item);
}


static const ber_old_sequence_t InformationCategories_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_InformationCategories_item },
};

static int
dissect_x420_InformationCategories(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          InformationCategories_sequence_of, hf_index, ett_x420_InformationCategories);

  return offset;
}



static int
dissect_x420_ManualHandlingInstruction(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x411_UniversalOrBMPString(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_ManualHandlingInstructions_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ManualHandlingInstruction(FALSE, tvb, offset, actx, tree, hf_x420_ManualHandlingInstructions_item);
}


static const ber_old_sequence_t ManualHandlingInstructions_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_ManualHandlingInstructions_item },
};

static int
dissect_x420_ManualHandlingInstructions(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          ManualHandlingInstructions_sequence_of, hf_index, ett_x420_ManualHandlingInstructions);

  return offset;
}



static int
dissect_x420_OriginatorsReference(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x411_UniversalOrBMPString(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}



static int
dissect_x420_PrecedencePolicyIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, actx, tree, tvb, offset, hf_index, NULL);

  return offset;
}



static int
dissect_x420_Precedence(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}



static int
dissect_x420_CharacterSetRegistration(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 170 "x420.cnf"
  guint32 crs;
  proto_item *pi;
    offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  &crs);


  if((pi = get_ber_last_created_item()))
    proto_item_append_text(pi, " (%s)", val_to_str(crs, charsetreg_vals, "unknown"));



  return offset;
}
static int dissect_GeneralTextParameters_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_CharacterSetRegistration(FALSE, tvb, offset, actx, tree, hf_x420_GeneralTextParameters_item);
}


static const ber_old_sequence_t GeneralTextParameters_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_INTEGER, BER_FLAGS_NOOWNTAG, dissect_GeneralTextParameters_item },
};

static int
dissect_x420_GeneralTextParameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     GeneralTextParameters_set_of, hf_index, ett_x420_GeneralTextParameters);

  return offset;
}



static int
dissect_x420_GeneralTextData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_GeneralString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}



static int
dissect_x420_IA5String(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_IA5String,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_supplementary_information_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_IA5String(TRUE, tvb, offset, actx, tree, hf_x420_supplementary_information);
}


static const ber_old_sequence_t VoiceParameters_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_voice_message_duration_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_voice_encoding_type_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_supplementary_information_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_VoiceParameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       VoiceParameters_sequence, hf_index, ett_x420_VoiceParameters);

  return offset;
}



static int
dissect_x420_VoiceData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}


static const ber_old_sequence_t SubmissionProof_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_proof_of_submission_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_originating_MTA_certificate_impl },
  { BER_CLASS_UNI, BER_UNI_TAG_SET, BER_FLAGS_NOOWNTAG, dissect_message_submission_envelope },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_SubmissionProof(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  SubmissionProof_set, hf_index, ett_x420_SubmissionProof);

  return offset;
}
static int dissect_submission_proof_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_SubmissionProof(TRUE, tvb, offset, actx, tree, hf_x420_submission_proof);
}


static const ber_old_sequence_t ForwardedContentParameters_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_delivery_time_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_delivery_envelope_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_mts_identifier_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_submission_proof_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_ForwardedContentParameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  ForwardedContentParameters_set, hf_index, ett_x420_ForwardedContentParameters);

  return offset;
}


static const value_string x420_Pathname_Attribute_vals[] = {
  {   0, "incomplete-pathname" },
  {  23, "complete-pathname" },
  { 0, NULL }
};

static const ber_old_choice_t Pathname_Attribute_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_incomplete_pathname_impl },
  {  23, BER_CLASS_CON, 23, BER_FLAGS_IMPLTAG, dissect_complete_pathname_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_Pathname_Attribute(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     Pathname_Attribute_choice, hf_index, ett_x420_Pathname_Attribute,
                                     NULL);

  return offset;
}
static int dissect_pathname(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Pathname_Attribute(FALSE, tvb, offset, actx, tree, hf_x420_pathname);
}
static int dissect_pathname_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Pathname_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_pathname);
}



static int
dissect_x420_GraphicString(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_GraphicString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_file_version_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_GraphicString(TRUE, tvb, offset, actx, tree, hf_x420_file_version);
}
static int dissect_descriptive_relationship_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_GraphicString(TRUE, tvb, offset, actx, tree, hf_x420_descriptive_relationship);
}
static int dissect_user_visible_string_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_GraphicString(FALSE, tvb, offset, actx, tree, hf_x420_user_visible_string_item);
}
static int dissect_descriptive_identifier_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_GraphicString(FALSE, tvb, offset, actx, tree, hf_x420_descriptive_identifier_item);
}
static int dissect_graphic_string(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_GraphicString(FALSE, tvb, offset, actx, tree, hf_x420_graphic_string);
}


static const ber_old_sequence_t PathnameandVersion_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_pathname_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_file_version_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_PathnameandVersion(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       PathnameandVersion_sequence, hf_index, ett_x420_PathnameandVersion);

  return offset;
}
static int dissect_pathname_and_version_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_PathnameandVersion(TRUE, tvb, offset, actx, tree, hf_x420_pathname_and_version);
}



static int
dissect_x420_OCTET_STRING(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_octet_string(implicit_tag, actx, tree, tvb, offset, hf_index,
                                       NULL);

  return offset;
}
static int dissect_application_cross_reference_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_OCTET_STRING(TRUE, tvb, offset, actx, tree, hf_x420_application_cross_reference);
}
static int dissect_octet_string(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_OCTET_STRING(FALSE, tvb, offset, actx, tree, hf_x420_octet_string);
}



static int
dissect_x420_PrintableString(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_PrintableString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_user_relative_identifier_ref_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_PrintableString(TRUE, tvb, offset, actx, tree, hf_x420_user_relative_identifier_ref);
}


static const ber_old_sequence_t MessageReference_set[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_user_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_user_relative_identifier_ref_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_MessageReference(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set(implicit_tag, actx, tree, tvb, offset,
                                  MessageReference_set, hf_index, ett_x420_MessageReference);

  return offset;
}
static int dissect_message_reference_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_MessageReference(TRUE, tvb, offset, actx, tree, hf_x420_message_reference);
}


static const ber_old_sequence_t CrossReference_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_application_cross_reference_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_message_reference_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_body_part_reference_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_CrossReference(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       CrossReference_sequence, hf_index, ett_x420_CrossReference);

  return offset;
}
static int dissect_cross_reference_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_CrossReference(TRUE, tvb, offset, actx, tree, hf_x420_cross_reference);
}


static const value_string x420_FileIdentifier_vals[] = {
  {   0, "pathname-and-version" },
  {   1, "cross-reference" },
  { 0, NULL }
};

static const ber_old_choice_t FileIdentifier_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_pathname_and_version_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_cross_reference_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_FileIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     FileIdentifier_choice, hf_index, ett_x420_FileIdentifier,
                                     NULL);

  return offset;
}
static int dissect_file_identifier(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_FileIdentifier(FALSE, tvb, offset, actx, tree, hf_x420_file_identifier);
}


static const value_string x420_ExplicitRelationship_vals[] = {
  {   0, "unspecified" },
  {   1, "new-file" },
  {   2, "replacement" },
  {   3, "extension" },
  { 0, NULL }
};


static int
dissect_x420_ExplicitRelationship(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_integer(implicit_tag, actx, tree, tvb, offset, hf_index,
                                  NULL);

  return offset;
}
static int dissect_explicit_relationship_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ExplicitRelationship(TRUE, tvb, offset, actx, tree, hf_x420_explicit_relationship);
}


static const value_string x420_Relationship_vals[] = {
  {   0, "explicit-relationship" },
  {   1, "descriptive-relationship" },
  { 0, NULL }
};

static const ber_old_choice_t Relationship_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_explicit_relationship_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_descriptive_relationship_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_Relationship(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     Relationship_choice, hf_index, ett_x420_Relationship,
                                     NULL);

  return offset;
}
static int dissect_relationship(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Relationship(FALSE, tvb, offset, actx, tree, hf_x420_relationship);
}


static const ber_old_sequence_t RelatedStoredFile_item_sequence[] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_file_identifier },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_relationship },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_RelatedStoredFile_item(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       RelatedStoredFile_item_sequence, hf_index, ett_x420_RelatedStoredFile_item);

  return offset;
}
static int dissect_RelatedStoredFile_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_RelatedStoredFile_item(FALSE, tvb, offset, actx, tree, hf_x420_RelatedStoredFile_item);
}


static const ber_old_sequence_t RelatedStoredFile_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_RelatedStoredFile_item },
};

static int
dissect_x420_RelatedStoredFile(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     RelatedStoredFile_set_of, hf_index, ett_x420_RelatedStoredFile);

  return offset;
}
static int dissect_related_stored_file_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_RelatedStoredFile(TRUE, tvb, offset, actx, tree, hf_x420_related_stored_file);
}



static int
dissect_x420_Document_Type_Name(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, actx, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_document_type_name(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Document_Type_Name(FALSE, tvb, offset, actx, tree, hf_x420_document_type_name);
}



static int
dissect_x420_T_parameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 184 "x420.cnf"
/* XXX: Not implemented yet */



  return offset;
}
static int dissect_parameter_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_T_parameter(TRUE, tvb, offset, actx, tree, hf_x420_parameter);
}


static const ber_old_sequence_t T_document_type_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_document_type_name },
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_parameter_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_T_document_type(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       T_document_type_sequence, hf_index, ett_x420_T_document_type);

  return offset;
}
static int dissect_document_type_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_T_document_type(TRUE, tvb, offset, actx, tree, hf_x420_document_type);
}



static int
dissect_x420_Constraint_Set_Name(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, actx, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_constraint_set_name(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Constraint_Set_Name(FALSE, tvb, offset, actx, tree, hf_x420_constraint_set_name);
}



static int
dissect_x420_Abstract_Syntax_Name(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_object_identifier(implicit_tag, actx, tree, tvb, offset, hf_index, NULL);

  return offset;
}
static int dissect_abstract_syntax_name(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Abstract_Syntax_Name(FALSE, tvb, offset, actx, tree, hf_x420_abstract_syntax_name);
}


static const ber_old_sequence_t T_constraint_set_and_abstract_syntax_sequence[] = {
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_constraint_set_name },
  { BER_CLASS_UNI, BER_UNI_TAG_OID, BER_FLAGS_NOOWNTAG, dissect_abstract_syntax_name },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_T_constraint_set_and_abstract_syntax(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       T_constraint_set_and_abstract_syntax_sequence, hf_index, ett_x420_T_constraint_set_and_abstract_syntax);

  return offset;
}
static int dissect_constraint_set_and_abstract_syntax_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_T_constraint_set_and_abstract_syntax(TRUE, tvb, offset, actx, tree, hf_x420_constraint_set_and_abstract_syntax);
}


static const value_string x420_Contents_Type_Attribute_vals[] = {
  {   0, "document-type" },
  {   1, "constraint-set-and-abstract-syntax" },
  { 0, NULL }
};

static const ber_old_choice_t Contents_Type_Attribute_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_document_type_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_constraint_set_and_abstract_syntax_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_Contents_Type_Attribute(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     Contents_Type_Attribute_choice, hf_index, ett_x420_Contents_Type_Attribute,
                                     NULL);

  return offset;
}



static int
dissect_x420_ContentsTypeParameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_x420_Contents_Type_Attribute(implicit_tag, tvb, offset, actx, tree, hf_index);

  return offset;
}
static int dissect_contents_type_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_ContentsTypeParameter(TRUE, tvb, offset, actx, tree, hf_x420_contents_type);
}


static const ber_old_sequence_t T_descriptive_identifier_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_GraphicString, BER_FLAGS_NOOWNTAG, dissect_descriptive_identifier_item },
};

static int
dissect_x420_T_descriptive_identifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          T_descriptive_identifier_sequence_of, hf_index, ett_x420_T_descriptive_identifier);

  return offset;
}
static int dissect_descriptive_identifier_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_T_descriptive_identifier(TRUE, tvb, offset, actx, tree, hf_x420_descriptive_identifier);
}


static const value_string x420_GeneralIdentifier_vals[] = {
  {   0, "registered-identifier" },
  {   1, "descriptive-identifier" },
  { 0, NULL }
};

static const ber_old_choice_t GeneralIdentifier_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_registered_identifier_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_descriptive_identifier_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_GeneralIdentifier(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     GeneralIdentifier_choice, hf_index, ett_x420_GeneralIdentifier,
                                     NULL);

  return offset;
}
static int dissect_application_reference_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_GeneralIdentifier(TRUE, tvb, offset, actx, tree, hf_x420_application_reference);
}
static int dissect_machine_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_GeneralIdentifier(TRUE, tvb, offset, actx, tree, hf_x420_machine);
}


static const ber_old_sequence_t T_user_visible_string_sequence_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_GraphicString, BER_FLAGS_NOOWNTAG, dissect_user_visible_string_item },
};

static int
dissect_x420_T_user_visible_string(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          T_user_visible_string_sequence_of, hf_index, ett_x420_T_user_visible_string);

  return offset;
}
static int dissect_user_visible_string_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_T_user_visible_string(TRUE, tvb, offset, actx, tree, hf_x420_user_visible_string);
}


static const ber_old_sequence_t EnvironmentParameter_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_application_reference_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_machine_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_operating_system_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_user_visible_string_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_EnvironmentParameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       EnvironmentParameter_sequence, hf_index, ett_x420_EnvironmentParameter);

  return offset;
}
static int dissect_environment_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_EnvironmentParameter(TRUE, tvb, offset, actx, tree, hf_x420_environment);
}



static int
dissect_x420_T_compression_algorithm_param(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
#line 187 "x420.cnf"
/* XXX: Not implemented yet */



  return offset;
}
static int dissect_compression_algorithm_param_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_T_compression_algorithm_param(TRUE, tvb, offset, actx, tree, hf_x420_compression_algorithm_param);
}


static const ber_old_sequence_t CompressionParameter_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_compression_algorithm_id_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_compression_algorithm_param_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_CompressionParameter(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       CompressionParameter_sequence, hf_index, ett_x420_CompressionParameter);

  return offset;
}
static int dissect_compression_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_CompressionParameter(TRUE, tvb, offset, actx, tree, hf_x420_compression);
}



static int
dissect_x420_Account(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_GraphicString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_account_actual_values(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Account(FALSE, tvb, offset, actx, tree, hf_x420_account_actual_values);
}


static const value_string x420_Account_Attribute_vals[] = {
  {   0, "no-value-available" },
  {   1, "actual-values" },
  { 0, NULL }
};

static const ber_old_choice_t Account_Attribute_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_no_value_available_impl },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_GraphicString, BER_FLAGS_NOOWNTAG, dissect_account_actual_values },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_Account_Attribute(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     Account_Attribute_choice, hf_index, ett_x420_Account_Attribute,
                                     NULL);

  return offset;
}
static int dissect_storage_account_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Account_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_storage_account);
}



static int
dissect_x420_User_Identity(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_restricted_string(implicit_tag, BER_UNI_TAG_GraphicString,
                                            actx, tree, tvb, offset, hf_index,
                                            NULL);

  return offset;
}
static int dissect_identity_actual_values(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_User_Identity(FALSE, tvb, offset, actx, tree, hf_x420_identity_actual_values);
}
static int dissect_identity_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_User_Identity(TRUE, tvb, offset, actx, tree, hf_x420_identity);
}


static const value_string x420_User_Identity_Attribute_vals[] = {
  {   0, "no-value-available" },
  {   1, "actual-values" },
  { 0, NULL }
};

static const ber_old_choice_t User_Identity_Attribute_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_no_value_available_impl },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_GraphicString, BER_FLAGS_NOOWNTAG, dissect_identity_actual_values },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_User_Identity_Attribute(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     User_Identity_Attribute_choice, hf_index, ett_x420_User_Identity_Attribute,
                                     NULL);

  return offset;
}
static int dissect_identity_of_creator_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_User_Identity_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_identity_of_creator);
}
static int dissect_identity_of_last_modifier_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_User_Identity_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_identity_of_last_modifier);
}
static int dissect_identity_of_last_reader_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_User_Identity_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_identity_of_last_reader);
}
static int dissect_identity_of_last_attribute_modifier_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_User_Identity_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_identity_of_last_attribute_modifier);
}


static const asn_namedbit Access_Request_bits[] = {
  {  0, &hf_x420_Access_Request_read, -1, -1, "read", NULL },
  {  1, &hf_x420_Access_Request_insert, -1, -1, "insert", NULL },
  {  2, &hf_x420_Access_Request_replace, -1, -1, "replace", NULL },
  {  3, &hf_x420_Access_Request_extend, -1, -1, "extend", NULL },
  {  4, &hf_x420_Access_Request_erase, -1, -1, "erase", NULL },
  {  5, &hf_x420_Access_Request_read_attribute, -1, -1, "read-attribute", NULL },
  {  6, &hf_x420_Access_Request_change_attribute, -1, -1, "change-attribute", NULL },
  {  7, &hf_x420_Access_Request_delete_object, -1, -1, "delete-object", NULL },
  { 0, NULL, 0, 0, NULL, NULL }
};

static int
dissect_x420_Access_Request(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_bitstring(implicit_tag, actx, tree, tvb, offset,
                                    Access_Request_bits, hf_index, ett_x420_Access_Request,
                                    NULL);

  return offset;
}
static int dissect_action_list_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Access_Request(TRUE, tvb, offset, actx, tree, hf_x420_action_list);
}


static const value_string x420_Password_vals[] = {
  {   0, "graphic-string" },
  {   1, "octet-string" },
  { 0, NULL }
};

static const ber_old_choice_t Password_choice[] = {
  {   0, BER_CLASS_UNI, BER_UNI_TAG_GraphicString, BER_FLAGS_NOOWNTAG, dissect_graphic_string },
  {   1, BER_CLASS_UNI, BER_UNI_TAG_OCTETSTRING, BER_FLAGS_NOOWNTAG, dissect_octet_string },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_Password(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     Password_choice, hf_index, ett_x420_Password,
                                     NULL);

  return offset;
}
static int dissect_read_password_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Password(TRUE, tvb, offset, actx, tree, hf_x420_read_password);
}
static int dissect_insert_password_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Password(TRUE, tvb, offset, actx, tree, hf_x420_insert_password);
}
static int dissect_replace_password_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Password(TRUE, tvb, offset, actx, tree, hf_x420_replace_password);
}
static int dissect_extend_password_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Password(TRUE, tvb, offset, actx, tree, hf_x420_extend_password);
}
static int dissect_erase_password_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Password(TRUE, tvb, offset, actx, tree, hf_x420_erase_password);
}
static int dissect_read_attribute_password_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Password(TRUE, tvb, offset, actx, tree, hf_x420_read_attribute_password);
}
static int dissect_change_attribute_password_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Password(TRUE, tvb, offset, actx, tree, hf_x420_change_attribute_password);
}
static int dissect_delete_password_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Password(TRUE, tvb, offset, actx, tree, hf_x420_delete_password);
}
static int dissect_link_password_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Password(TRUE, tvb, offset, actx, tree, hf_x420_link_password);
}
static int dissect_Pass_Passwords_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Password(FALSE, tvb, offset, actx, tree, hf_x420_Pass_Passwords_item);
}


static const ber_old_sequence_t Pass_Passwords_sequence_of[1] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_Pass_Passwords_item },
};

static int
dissect_x420_Pass_Passwords(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          Pass_Passwords_sequence_of, hf_index, ett_x420_Pass_Passwords);

  return offset;
}
static int dissect_pass_passwords_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Pass_Passwords(TRUE, tvb, offset, actx, tree, hf_x420_pass_passwords);
}


static const ber_old_sequence_t Access_Passwords_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_read_password_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_insert_password_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_replace_password_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_extend_password_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_erase_password_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_read_attribute_password_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_change_attribute_password_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_delete_password_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_IMPLTAG, dissect_pass_passwords_impl },
  { BER_CLASS_CON, 9, BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_link_password_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_Access_Passwords(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Access_Passwords_sequence, hf_index, ett_x420_Access_Passwords);

  return offset;
}
static int dissect_passwords_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Access_Passwords(TRUE, tvb, offset, actx, tree, hf_x420_passwords);
}


static const ber_old_sequence_t Application_Entity_Title_sequence[] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_ap_title },
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_NOOWNTAG, dissect_ae_qualifier },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_Application_Entity_Title(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Application_Entity_Title_sequence, hf_index, ett_x420_Application_Entity_Title);

  return offset;
}
static int dissect_location_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Application_Entity_Title(TRUE, tvb, offset, actx, tree, hf_x420_location);
}


static const ber_old_sequence_t Access_Control_Element_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_action_list_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_concurrency_access_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_identity_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_passwords_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_location_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_Access_Control_Element(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       Access_Control_Element_sequence, hf_index, ett_x420_Access_Control_Element);

  return offset;
}
static int dissect_actual_values_item(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Access_Control_Element(FALSE, tvb, offset, actx, tree, hf_x420_actual_values_item);
}


static const ber_old_sequence_t SET_OF_Access_Control_Element_set_of[1] = {
  { BER_CLASS_UNI, BER_UNI_TAG_SEQUENCE, BER_FLAGS_NOOWNTAG, dissect_actual_values_item },
};

static int
dissect_x420_SET_OF_Access_Control_Element(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_set_of(implicit_tag, actx, tree, tvb, offset,
                                     SET_OF_Access_Control_Element_set_of, hf_index, ett_x420_SET_OF_Access_Control_Element);

  return offset;
}
static int dissect_actual_values_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_SET_OF_Access_Control_Element(TRUE, tvb, offset, actx, tree, hf_x420_actual_values);
}


static const value_string x420_Access_Control_Attribute_vals[] = {
  {   0, "no-value-available" },
  {   1, "actual-values" },
  { 0, NULL }
};

static const ber_old_choice_t Access_Control_Attribute_choice[] = {
  {   0, BER_CLASS_CON, 0, BER_FLAGS_IMPLTAG, dissect_no_value_available_impl },
  {   1, BER_CLASS_CON, 1, BER_FLAGS_IMPLTAG, dissect_actual_values_impl },
  { 0, 0, 0, 0, NULL }
};

static int
dissect_x420_Access_Control_Attribute(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_choice(actx, tree, tvb, offset,
                                     Access_Control_Attribute_choice, hf_index, ett_x420_Access_Control_Attribute,
                                     NULL);

  return offset;
}
static int dissect_access_control_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_Access_Control_Attribute(TRUE, tvb, offset, actx, tree, hf_x420_access_control);
}


static const ber_old_sequence_t FileAttributes_sequence[] = {
  { BER_CLASS_ANY/*choice*/, -1/*choice*/, BER_FLAGS_OPTIONAL|BER_FLAGS_NOOWNTAG|BER_FLAGS_NOTCHKTAG, dissect_pathname },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_permitted_actions_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_storage_account_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_date_and_time_of_creation_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_date_and_time_of_last_modification_impl },
  { BER_CLASS_CON, 6, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_date_and_time_of_last_read_access_impl },
  { BER_CLASS_CON, 7, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_date_and_time_of_last_attribute_modification_impl },
  { BER_CLASS_CON, 8, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_identity_of_creator_impl },
  { BER_CLASS_CON, 9, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_identity_of_last_modifier_impl },
  { BER_CLASS_CON, 10, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_identity_of_last_reader_impl },
  { BER_CLASS_CON, 11, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_identity_of_last_attribute_modifier_impl },
  { BER_CLASS_CON, 12, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_object_availability_impl },
  { BER_CLASS_CON, 13, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_object_size_impl },
  { BER_CLASS_CON, 14, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_future_object_size_impl },
  { BER_CLASS_CON, 15, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_access_control_impl },
  { BER_CLASS_CON, 16, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_legal_qualifications_impl },
  { BER_CLASS_CON, 17, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_private_use_impl },
  { BER_CLASS_CON, 22, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_attribute_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_FileAttributes(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       FileAttributes_sequence, hf_index, ett_x420_FileAttributes);

  return offset;
}
static int dissect_file_attributes_impl(proto_tree *tree _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_) {
  return dissect_x420_FileAttributes(TRUE, tvb, offset, actx, tree, hf_x420_file_attributes);
}


static const ber_old_sequence_t FileTransferParameters_sequence[] = {
  { BER_CLASS_CON, 0, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_related_stored_file_impl },
  { BER_CLASS_CON, 1, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG|BER_FLAGS_NOTCHKTAG, dissect_contents_type_impl },
  { BER_CLASS_CON, 2, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_environment_impl },
  { BER_CLASS_CON, 3, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_compression_impl },
  { BER_CLASS_CON, 4, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_file_attributes_impl },
  { BER_CLASS_CON, 5, BER_FLAGS_OPTIONAL|BER_FLAGS_IMPLTAG, dissect_extensions_impl },
  { 0, 0, 0, NULL }
};

static int
dissect_x420_FileTransferParameters(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence(implicit_tag, actx, tree, tvb, offset,
                                       FileTransferParameters_sequence, hf_index, ett_x420_FileTransferParameters);

  return offset;
}


static const ber_old_sequence_t FileTransferData_sequence_of[1] = {
  { BER_CLASS_UNI, 8, BER_FLAGS_NOOWNTAG, dissect_FileTransferData_item },
};

static int
dissect_x420_FileTransferData(gboolean implicit_tag _U_, tvbuff_t *tvb _U_, int offset _U_, asn1_ctx_t *actx _U_, proto_tree *tree _U_, int hf_index _U_) {
  offset = dissect_ber_old_sequence_of(implicit_tag, actx, tree, tvb, offset,
                                          FileTransferData_sequence_of, hf_index, ett_x420_FileTransferData);

  return offset;
}

/*--- PDUs ---*/

static void dissect_InformationObject_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_InformationObject(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_InformationObject_PDU);
}
static void dissect_IA5TextParameters_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_IA5TextParameters(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_IA5TextParameters_PDU);
}
static void dissect_IA5TextData_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_IA5TextData(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_IA5TextData_PDU);
}
static void dissect_G3FacsimileParameters_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_G3FacsimileParameters(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_G3FacsimileParameters_PDU);
}
static void dissect_G3FacsimileData_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_G3FacsimileData(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_G3FacsimileData_PDU);
}
static void dissect_G4Class1Data_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_G4Class1Data(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_G4Class1Data_PDU);
}
static void dissect_MixedModeData_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_MixedModeData(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_MixedModeData_PDU);
}
static void dissect_TeletexParameters_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_TeletexParameters(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_TeletexParameters_PDU);
}
static void dissect_TeletexData_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_TeletexData(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_TeletexData_PDU);
}
static void dissect_VideotexParameters_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_VideotexParameters(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_VideotexParameters_PDU);
}
static void dissect_VideotexData_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_VideotexData(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_VideotexData_PDU);
}
static void dissect_EncryptedParameters_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_EncryptedParameters(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_EncryptedParameters_PDU);
}
static void dissect_EncryptedData_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_EncryptedData(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_EncryptedData_PDU);
}
static void dissect_MessageParameters_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_MessageParameters(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_MessageParameters_PDU);
}
static void dissect_MessageData_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_MessageData(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_MessageData_PDU);
}
static void dissect_BilaterallyDefinedBodyPart_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_BilaterallyDefinedBodyPart(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_BilaterallyDefinedBodyPart_PDU);
}
static void dissect_IPN_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_IPN(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_IPN_PDU);
}
static void dissect_AbsenceAdvice_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_AbsenceAdvice(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_AbsenceAdvice_PDU);
}
static void dissect_ChangeOfAddressAdvice_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_ChangeOfAddressAdvice(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_ChangeOfAddressAdvice_PDU);
}
static void dissect_IPMAssemblyInstructions_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_IPMAssemblyInstructions(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_IPMAssemblyInstructions_PDU);
}
static void dissect_OriginatingUA_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_OriginatingUA(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_OriginatingUA_PDU);
}
static void dissect_IncompleteCopy_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_IncompleteCopy(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_IncompleteCopy_PDU);
}
static void dissect_Languages_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_Languages(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_Languages_PDU);
}
static void dissect_AutoSubmitted_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_AutoSubmitted(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_AutoSubmitted_PDU);
}
static void dissect_BodyPartSignatures_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_BodyPartSignatures(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_BodyPartSignatures_PDU);
}
static void dissect_IPMSecurityLabel_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_IPMSecurityLabel(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_IPMSecurityLabel_PDU);
}
static void dissect_AuthorizationTime_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_AuthorizationTime(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_AuthorizationTime_PDU);
}
static void dissect_CirculationList_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_CirculationList(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_CirculationList_PDU);
}
static void dissect_CirculationListIndicator_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_CirculationListIndicator(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_CirculationListIndicator_PDU);
}
static void dissect_DistributionCodes_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_DistributionCodes(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_DistributionCodes_PDU);
}
static void dissect_ExtendedSubject_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_ExtendedSubject(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_ExtendedSubject_PDU);
}
static void dissect_InformationCategories_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_InformationCategories(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_InformationCategories_PDU);
}
static void dissect_ManualHandlingInstructions_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_ManualHandlingInstructions(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_ManualHandlingInstructions_PDU);
}
static void dissect_OriginatorsReference_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_OriginatorsReference(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_OriginatorsReference_PDU);
}
static void dissect_PrecedencePolicyIdentifier_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_PrecedencePolicyIdentifier(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_PrecedencePolicyIdentifier_PDU);
}
static void dissect_Precedence_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_Precedence(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_Precedence_PDU);
}
static void dissect_GeneralTextParameters_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_GeneralTextParameters(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_GeneralTextParameters_PDU);
}
static void dissect_GeneralTextData_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_GeneralTextData(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_GeneralTextData_PDU);
}
static void dissect_VoiceParameters_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_VoiceParameters(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_VoiceParameters_PDU);
}
static void dissect_VoiceData_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_VoiceData(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_VoiceData_PDU);
}
static void dissect_ForwardedContentParameters_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_ForwardedContentParameters(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_ForwardedContentParameters_PDU);
}
static void dissect_FileTransferParameters_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_FileTransferParameters(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_FileTransferParameters_PDU);
}
static void dissect_FileTransferData_PDU(tvbuff_t *tvb _U_, packet_info *pinfo _U_, proto_tree *tree _U_) {
  asn1_ctx_t asn1_ctx;
  asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);
  dissect_x420_FileTransferData(FALSE, tvb, 0, &asn1_ctx, tree, hf_x420_FileTransferData_PDU);
}


/*--- End of included file: packet-x420-fn.c ---*/
#line 95 "packet-x420-template.c"

/*
* Dissect X420 PDUs inside a PPDU.
*/
static void
dissect_x420(tvbuff_t *tvb, packet_info *pinfo, proto_tree *parent_tree)
{
	int offset = 0;
	proto_item *item=NULL;
	proto_tree *tree=NULL;
	asn1_ctx_t asn1_ctx;
	asn1_ctx_init(&asn1_ctx, ASN1_ENC_BER, TRUE, pinfo);

	if(parent_tree){
		item = proto_tree_add_item(parent_tree, proto_x420, tvb, 0, -1, FALSE);
		tree = proto_item_add_subtree(item, ett_x420);
	}

	if (check_col(pinfo->cinfo, COL_PROTOCOL))
		col_set_str(pinfo->cinfo, COL_PROTOCOL, "P22");
	if (check_col(pinfo->cinfo, COL_INFO))
	  col_add_str(pinfo->cinfo, COL_INFO, "InterPersonal");

	dissect_x420_InformationObject(TRUE, tvb, offset, &asn1_ctx , tree, -1);
}


/*--- proto_register_x420 -------------------------------------------*/
void proto_register_x420(void) {

  /* List of fields */
  static hf_register_info hf[] =
  {

/*--- Included file: packet-x420-hfarr.c ---*/
#line 1 "packet-x420-hfarr.c"
    { &hf_x420_InformationObject_PDU,
      { "InformationObject", "x420.InformationObject",
        FT_UINT32, BASE_DEC, VALS(x420_InformationObject_vals), 0,
        "x420.InformationObject", HFILL }},
    { &hf_x420_IA5TextParameters_PDU,
      { "IA5TextParameters", "x420.IA5TextParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IA5TextParameters", HFILL }},
    { &hf_x420_IA5TextData_PDU,
      { "IA5TextData", "x420.IA5TextData",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.IA5TextData", HFILL }},
    { &hf_x420_G3FacsimileParameters_PDU,
      { "G3FacsimileParameters", "x420.G3FacsimileParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.G3FacsimileParameters", HFILL }},
    { &hf_x420_G3FacsimileData_PDU,
      { "G3FacsimileData", "x420.G3FacsimileData",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.G3FacsimileData", HFILL }},
    { &hf_x420_G4Class1Data_PDU,
      { "G4Class1Data", "x420.G4Class1Data",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.G4Class1Data", HFILL }},
    { &hf_x420_MixedModeData_PDU,
      { "MixedModeData", "x420.MixedModeData",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.MixedModeData", HFILL }},
    { &hf_x420_TeletexParameters_PDU,
      { "TeletexParameters", "x420.TeletexParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.TeletexParameters", HFILL }},
    { &hf_x420_TeletexData_PDU,
      { "TeletexData", "x420.TeletexData",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.TeletexData", HFILL }},
    { &hf_x420_VideotexParameters_PDU,
      { "VideotexParameters", "x420.VideotexParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.VideotexParameters", HFILL }},
    { &hf_x420_VideotexData_PDU,
      { "VideotexData", "x420.VideotexData",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.VideotexData", HFILL }},
    { &hf_x420_EncryptedParameters_PDU,
      { "EncryptedParameters", "x420.EncryptedParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.EncryptedParameters", HFILL }},
    { &hf_x420_EncryptedData_PDU,
      { "EncryptedData", "x420.EncryptedData",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x420.EncryptedData", HFILL }},
    { &hf_x420_MessageParameters_PDU,
      { "MessageParameters", "x420.MessageParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.MessageParameters", HFILL }},
    { &hf_x420_MessageData_PDU,
      { "MessageData", "x420.MessageData",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.MessageData", HFILL }},
    { &hf_x420_BilaterallyDefinedBodyPart_PDU,
      { "BilaterallyDefinedBodyPart", "x420.BilaterallyDefinedBodyPart",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x420.BilaterallyDefinedBodyPart", HFILL }},
    { &hf_x420_IPN_PDU,
      { "IPN", "x420.IPN",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPN", HFILL }},
    { &hf_x420_AbsenceAdvice_PDU,
      { "AbsenceAdvice", "x420.AbsenceAdvice",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.AbsenceAdvice", HFILL }},
    { &hf_x420_ChangeOfAddressAdvice_PDU,
      { "ChangeOfAddressAdvice", "x420.ChangeOfAddressAdvice",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ChangeOfAddressAdvice", HFILL }},
    { &hf_x420_IPMAssemblyInstructions_PDU,
      { "IPMAssemblyInstructions", "x420.IPMAssemblyInstructions",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPMAssemblyInstructions", HFILL }},
    { &hf_x420_OriginatingUA_PDU,
      { "OriginatingUA", "x420.OriginatingUA",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.OriginatingUA", HFILL }},
    { &hf_x420_IncompleteCopy_PDU,
      { "IncompleteCopy", "x420.IncompleteCopy",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IncompleteCopy", HFILL }},
    { &hf_x420_Languages_PDU,
      { "Languages", "x420.Languages",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.Languages", HFILL }},
    { &hf_x420_AutoSubmitted_PDU,
      { "AutoSubmitted", "x420.AutoSubmitted",
        FT_UINT32, BASE_DEC, VALS(x420_AutoSubmitted_vals), 0,
        "x420.AutoSubmitted", HFILL }},
    { &hf_x420_BodyPartSignatures_PDU,
      { "BodyPartSignatures", "x420.BodyPartSignatures",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.BodyPartSignatures", HFILL }},
    { &hf_x420_IPMSecurityLabel_PDU,
      { "IPMSecurityLabel", "x420.IPMSecurityLabel",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPMSecurityLabel", HFILL }},
    { &hf_x420_AuthorizationTime_PDU,
      { "AuthorizationTime", "x420.AuthorizationTime",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.AuthorizationTime", HFILL }},
    { &hf_x420_CirculationList_PDU,
      { "CirculationList", "x420.CirculationList",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.CirculationList", HFILL }},
    { &hf_x420_CirculationListIndicator_PDU,
      { "CirculationListIndicator", "x420.CirculationListIndicator",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.CirculationListIndicator", HFILL }},
    { &hf_x420_DistributionCodes_PDU,
      { "DistributionCodes", "x420.DistributionCodes",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.DistributionCodes", HFILL }},
    { &hf_x420_ExtendedSubject_PDU,
      { "ExtendedSubject", "x420.ExtendedSubject",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ExtendedSubject", HFILL }},
    { &hf_x420_InformationCategories_PDU,
      { "InformationCategories", "x420.InformationCategories",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.InformationCategories", HFILL }},
    { &hf_x420_ManualHandlingInstructions_PDU,
      { "ManualHandlingInstructions", "x420.ManualHandlingInstructions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.ManualHandlingInstructions", HFILL }},
    { &hf_x420_OriginatorsReference_PDU,
      { "OriginatorsReference", "x420.OriginatorsReference",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.OriginatorsReference", HFILL }},
    { &hf_x420_PrecedencePolicyIdentifier_PDU,
      { "PrecedencePolicyIdentifier", "x420.PrecedencePolicyIdentifier",
        FT_OID, BASE_NONE, NULL, 0,
        "x420.PrecedencePolicyIdentifier", HFILL }},
    { &hf_x420_Precedence_PDU,
      { "Precedence", "x420.Precedence",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.Precedence", HFILL }},
    { &hf_x420_GeneralTextParameters_PDU,
      { "GeneralTextParameters", "x420.GeneralTextParameters",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.GeneralTextParameters", HFILL }},
    { &hf_x420_GeneralTextData_PDU,
      { "GeneralTextData", "x420.GeneralTextData",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.GeneralTextData", HFILL }},
    { &hf_x420_VoiceParameters_PDU,
      { "VoiceParameters", "x420.VoiceParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.VoiceParameters", HFILL }},
    { &hf_x420_VoiceData_PDU,
      { "VoiceData", "x420.VoiceData",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x420.VoiceData", HFILL }},
    { &hf_x420_ForwardedContentParameters_PDU,
      { "ForwardedContentParameters", "x420.ForwardedContentParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ForwardedContentParameters", HFILL }},
    { &hf_x420_FileTransferParameters_PDU,
      { "FileTransferParameters", "x420.FileTransferParameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.FileTransferParameters", HFILL }},
    { &hf_x420_FileTransferData_PDU,
      { "FileTransferData", "x420.FileTransferData",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.FileTransferData", HFILL }},
    { &hf_x420_ipm,
      { "ipm", "x420.ipm",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPM", HFILL }},
    { &hf_x420_ipn,
      { "ipn", "x420.ipn",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPN", HFILL }},
    { &hf_x420_heading,
      { "heading", "x420.heading",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.Heading", HFILL }},
    { &hf_x420_body,
      { "body", "x420.body",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.Body", HFILL }},
    { &hf_x420_type,
      { "type", "x420.type",
        FT_OID, BASE_NONE, NULL, 0,
        "x420.T_type", HFILL }},
    { &hf_x420_value,
      { "value", "x420.value",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.T_value", HFILL }},
    { &hf_x420_this_IPM,
      { "this-IPM", "x420.this_IPM",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ThisIPMField", HFILL }},
    { &hf_x420_originator,
      { "originator", "x420.originator",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.OriginatorField", HFILL }},
    { &hf_x420_authorizing_users,
      { "authorizing-users", "x420.authorizing_users",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.AuthorizingUsersField", HFILL }},
    { &hf_x420_primary_recipients,
      { "primary-recipients", "x420.primary_recipients",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.PrimaryRecipientsField", HFILL }},
    { &hf_x420_copy_recipients,
      { "copy-recipients", "x420.copy_recipients",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.CopyRecipientsField", HFILL }},
    { &hf_x420_blind_copy_recipients,
      { "blind-copy-recipients", "x420.blind_copy_recipients",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.BlindCopyRecipientsField", HFILL }},
    { &hf_x420_replied_to_IPM,
      { "replied-to-IPM", "x420.replied_to_IPM",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.RepliedToIPMField", HFILL }},
    { &hf_x420_obsoleted_IPMs,
      { "obsoleted-IPMs", "x420.obsoleted_IPMs",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.ObsoletedIPMsField", HFILL }},
    { &hf_x420_related_IPMs,
      { "related-IPMs", "x420.related_IPMs",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.RelatedIPMsField", HFILL }},
    { &hf_x420_subject,
      { "subject", "x420.subject",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.SubjectField", HFILL }},
    { &hf_x420_expiry_time,
      { "expiry-time", "x420.expiry_time",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.ExpiryTimeField", HFILL }},
    { &hf_x420_reply_time,
      { "reply-time", "x420.reply_time",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.ReplyTimeField", HFILL }},
    { &hf_x420_reply_recipients,
      { "reply-recipients", "x420.reply_recipients",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.ReplyRecipientsField", HFILL }},
    { &hf_x420_importance,
      { "importance", "x420.importance",
        FT_UINT32, BASE_DEC, VALS(x420_ImportanceField_vals), 0,
        "x420.ImportanceField", HFILL }},
    { &hf_x420_sensitivity,
      { "sensitivity", "x420.sensitivity",
        FT_UINT32, BASE_DEC, VALS(x420_SensitivityField_vals), 0,
        "x420.SensitivityField", HFILL }},
    { &hf_x420_auto_forwarded,
      { "auto-forwarded", "x420.auto_forwarded",
        FT_BOOLEAN, 8, NULL, 0,
        "x420.AutoForwardedField", HFILL }},
    { &hf_x420_extensions,
      { "extensions", "x420.extensions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.ExtensionsField", HFILL }},
    { &hf_x420_user,
      { "user", "x420.user",
        FT_NONE, BASE_NONE, NULL, 0,
        "x411.ORName", HFILL }},
    { &hf_x420_user_relative_identifier,
      { "user-relative-identifier", "x420.user_relative_identifier",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.LocalIPMIdentifier", HFILL }},
    { &hf_x420_recipient,
      { "recipient", "x420.recipient",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ORDescriptor", HFILL }},
    { &hf_x420_notification_requests,
      { "notification-requests", "x420.notification_requests",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x420.NotificationRequests", HFILL }},
    { &hf_x420_reply_requested,
      { "reply-requested", "x420.reply_requested",
        FT_BOOLEAN, 8, NULL, 0,
        "x420.BOOLEAN", HFILL }},
    { &hf_x420_recipient_extensions,
      { "recipient-extensions", "x420.recipient_extensions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.RecipientExtensionsField", HFILL }},
    { &hf_x420_formal_name,
      { "formal-name", "x420.formal_name",
        FT_NONE, BASE_NONE, NULL, 0,
        "x411.ORName", HFILL }},
    { &hf_x420_free_form_name,
      { "free-form-name", "x420.free_form_name",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.FreeFormName", HFILL }},
    { &hf_x420_telephone_number,
      { "telephone-number", "x420.telephone_number",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.TelephoneNumber", HFILL }},
    { &hf_x420_RecipientExtensionsField_item,
      { "Item", "x420.RecipientExtensionsField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPMSExtension", HFILL }},
    { &hf_x420_AuthorizingUsersField_item,
      { "Item", "x420.AuthorizingUsersField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.AuthorizingUsersSubfield", HFILL }},
    { &hf_x420_PrimaryRecipientsField_item,
      { "Item", "x420.PrimaryRecipientsField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.PrimaryRecipientsSubfield", HFILL }},
    { &hf_x420_CopyRecipientsField_item,
      { "Item", "x420.CopyRecipientsField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.CopyRecipientsSubfield", HFILL }},
    { &hf_x420_BlindCopyRecipientsField_item,
      { "Item", "x420.BlindCopyRecipientsField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.BlindCopyRecipientsSubfield", HFILL }},
    { &hf_x420_ObsoletedIPMsField_item,
      { "Item", "x420.ObsoletedIPMsField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ObsoletedIPMsSubfield", HFILL }},
    { &hf_x420_RelatedIPMsField_item,
      { "Item", "x420.RelatedIPMsField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.RelatedIPMsSubfield", HFILL }},
    { &hf_x420_ReplyRecipientsField_item,
      { "Item", "x420.ReplyRecipientsField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ReplyRecipientsSubfield", HFILL }},
    { &hf_x420_ExtensionsField_item,
      { "Item", "x420.ExtensionsField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPMSExtension", HFILL }},
    { &hf_x420_Body_item,
      { "Item", "x420.Body_item",
        FT_UINT32, BASE_DEC, VALS(x420_BodyPart_vals), 0,
        "x420.BodyPart", HFILL }},
    { &hf_x420_ia5_text,
      { "ia5-text", "x420.ia5_text",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IA5TextBodyPart", HFILL }},
    { &hf_x420_g3_facsimile,
      { "g3-facsimile", "x420.g3_facsimile",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.G3FacsimileBodyPart", HFILL }},
    { &hf_x420_g4_class1,
      { "g4-class1", "x420.g4_class1",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.G4Class1BodyPart", HFILL }},
    { &hf_x420_teletex,
      { "teletex", "x420.teletex",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.TeletexBodyPart", HFILL }},
    { &hf_x420_videotex,
      { "videotex", "x420.videotex",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.VideotexBodyPart", HFILL }},
    { &hf_x420_encrypted_bp,
      { "encrypted", "x420.encrypted",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.EncryptedBodyPart", HFILL }},
    { &hf_x420_message,
      { "message", "x420.message",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.MessageBodyPart", HFILL }},
    { &hf_x420_mixed_mode,
      { "mixed-mode", "x420.mixed_mode",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.MixedModeBodyPart", HFILL }},
    { &hf_x420_bilaterally_defined,
      { "bilaterally-defined", "x420.bilaterally_defined",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x420.BilaterallyDefinedBodyPart", HFILL }},
    { &hf_x420_nationally_defined,
      { "nationally-defined", "x420.nationally_defined",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.NationallyDefinedBodyPart", HFILL }},
    { &hf_x420_extended,
      { "extended", "x420.extended",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ExtendedBodyPart", HFILL }},
    { &hf_x420_extended_parameters,
      { "parameters", "x420.parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "acse.EXTERNALt", HFILL }},
    { &hf_x420_extended_data,
      { "data", "x420.data",
        FT_NONE, BASE_NONE, NULL, 0,
        "acse.EXTERNALt", HFILL }},
    { &hf_x420_ia5text_parameters,
      { "parameters", "x420.parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IA5TextParameters", HFILL }},
    { &hf_x420_ia5text_data,
      { "data", "x420.data",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.IA5TextData", HFILL }},
    { &hf_x420_repertoire,
      { "repertoire", "x420.repertoire",
        FT_UINT32, BASE_DEC, VALS(x420_Repertoire_vals), 0,
        "x420.Repertoire", HFILL }},
    { &hf_x420_g3facsimile_parameters,
      { "parameters", "x420.parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.G3FacsimileParameters", HFILL }},
    { &hf_x420_g3facsimile_data,
      { "data", "x420.data",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.G3FacsimileData", HFILL }},
    { &hf_x420_number_of_pages,
      { "number-of-pages", "x420.number_of_pages",
        FT_INT32, BASE_DEC, NULL, 0,
        "x420.INTEGER", HFILL }},
    { &hf_x420_g3facsimile_non_basic_parameters,
      { "non-basic-parameters", "x420.non_basic_parameters",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x411.G3FacsimileNonBasicParameters", HFILL }},
    { &hf_x420_G3FacsimileData_item,
      { "Item", "x420.G3FacsimileData_item",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x420.BIT_STRING", HFILL }},
    { &hf_x420_G4Class1Data_item,
      { "Item", "x420.G4Class1Data_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.Interchange_Data_Element", HFILL }},
    { &hf_x420_MixedModeData_item,
      { "Item", "x420.MixedModeData_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.Interchange_Data_Element", HFILL }},
    { &hf_x420_teletex_parameters,
      { "parameters", "x420.parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.TeletexParameters", HFILL }},
    { &hf_x420_teletex_data,
      { "data", "x420.data",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.TeletexData", HFILL }},
    { &hf_x420_telex_compatible,
      { "telex-compatible", "x420.telex_compatible",
        FT_BOOLEAN, 8, NULL, 0,
        "x420.BOOLEAN", HFILL }},
    { &hf_x420_teletex_non_basic_parameters,
      { "non-basic-parameters", "x420.non_basic_parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x411.TeletexNonBasicParameters", HFILL }},
    { &hf_x420_TeletexData_item,
      { "Item", "x420.TeletexData_item",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.TeletexString", HFILL }},
    { &hf_x420_videotex_parameters,
      { "parameters", "x420.parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.VideotexParameters", HFILL }},
    { &hf_x420_videotex_data,
      { "data", "x420.data",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.VideotexData", HFILL }},
    { &hf_x420_syntax,
      { "syntax", "x420.syntax",
        FT_INT32, BASE_DEC, VALS(x420_VideotexSyntax_vals), 0,
        "x420.VideotexSyntax", HFILL }},
    { &hf_x420_encrypted_parameters,
      { "parameters", "x420.parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.EncryptedParameters", HFILL }},
    { &hf_x420_encrypted_data,
      { "data", "x420.data",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x420.EncryptedData", HFILL }},
    { &hf_x420_algorithm_identifier,
      { "algorithm-identifier", "x420.algorithm_identifier",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.AlgorithmIdentifier", HFILL }},
    { &hf_x420_originator_certificates,
      { "originator-certificates", "x420.originator_certificates",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x411.ExtendedCertificates", HFILL }},
    { &hf_x420_message_parameters,
      { "parameters", "x420.parameters",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.MessageParameters", HFILL }},
    { &hf_x420_message_data,
      { "data", "x420.data",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.MessageData", HFILL }},
    { &hf_x420_delivery_time,
      { "delivery-time", "x420.delivery_time",
        FT_STRING, BASE_NONE, NULL, 0,
        "x411.MessageDeliveryTime", HFILL }},
    { &hf_x420_delivery_envelope,
      { "delivery-envelope", "x420.delivery_envelope",
        FT_NONE, BASE_NONE, NULL, 0,
        "x411.OtherMessageDeliveryFields", HFILL }},
    { &hf_x420_subject_ipm,
      { "subject-ipm", "x420.subject_ipm",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.SubjectIPMField", HFILL }},
    { &hf_x420_ipn_originator,
      { "ipn-originator", "x420.ipn_originator",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPNOriginatorField", HFILL }},
    { &hf_x420_ipm_intended_recipient,
      { "ipm-intended-recipient", "x420.ipm_intended_recipient",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPMIntendedRecipientField", HFILL }},
    { &hf_x420_conversion_eits,
      { "conversion-eits", "x420.conversion_eits",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ConversionEITsField", HFILL }},
    { &hf_x420_notification_extensions,
      { "notification-extensions", "x420.notification_extensions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.NotificationExtensionsField", HFILL }},
    { &hf_x420_choice,
      { "choice", "x420.choice",
        FT_UINT32, BASE_DEC, VALS(x420_T_choice_vals), 0,
        "x420.T_choice", HFILL }},
    { &hf_x420_non_receipt_fields,
      { "non-receipt-fields", "x420.non_receipt_fields",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.NonReceiptFields", HFILL }},
    { &hf_x420_receipt_fields,
      { "receipt-fields", "x420.receipt_fields",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ReceiptFields", HFILL }},
    { &hf_x420_other_notification_type_fields,
      { "other-notification-type-fields", "x420.other_notification_type_fields",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.OtherNotificationTypeFields", HFILL }},
    { &hf_x420_non_receipt_reason,
      { "non-receipt-reason", "x420.non_receipt_reason",
        FT_UINT32, BASE_DEC, VALS(x420_NonReceiptReasonField_vals), 0,
        "x420.NonReceiptReasonField", HFILL }},
    { &hf_x420_discard_reason,
      { "discard-reason", "x420.discard_reason",
        FT_UINT32, BASE_DEC, VALS(x420_DiscardReasonField_vals), 0,
        "x420.DiscardReasonField", HFILL }},
    { &hf_x420_auto_forward_comment,
      { "auto-forward-comment", "x420.auto_forward_comment",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.AutoForwardCommentField", HFILL }},
    { &hf_x420_returned_ipm,
      { "returned-ipm", "x420.returned_ipm",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ReturnedIPMField", HFILL }},
    { &hf_x420_nrn_extensions,
      { "nrn-extensions", "x420.nrn_extensions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.NRNExtensionsField", HFILL }},
    { &hf_x420_receipt_time,
      { "receipt-time", "x420.receipt_time",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.ReceiptTimeField", HFILL }},
    { &hf_x420_acknowledgment_mode,
      { "acknowledgment-mode", "x420.acknowledgment_mode",
        FT_UINT32, BASE_DEC, VALS(x420_AcknowledgmentModeField_vals), 0,
        "x420.AcknowledgmentModeField", HFILL }},
    { &hf_x420_suppl_receipt_info,
      { "suppl-receipt-info", "x420.suppl_receipt_info",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.SupplReceiptInfoField", HFILL }},
    { &hf_x420_rn_extensions,
      { "rn-extensions", "x420.rn_extensions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.RNExtensionsField", HFILL }},
    { &hf_x420_NotificationExtensionsField_item,
      { "Item", "x420.NotificationExtensionsField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPMSExtension", HFILL }},
    { &hf_x420_NRNExtensionsField_item,
      { "Item", "x420.NRNExtensionsField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPMSExtension", HFILL }},
    { &hf_x420_RNExtensionsField_item,
      { "Item", "x420.RNExtensionsField_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPMSExtension", HFILL }},
    { &hf_x420_OtherNotificationTypeFields_item,
      { "Item", "x420.OtherNotificationTypeFields_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.IPMSExtension", HFILL }},
    { &hf_x420_advice,
      { "advice", "x420.advice",
        FT_UINT32, BASE_DEC, VALS(x420_BodyPart_vals), 0,
        "x420.BodyPart", HFILL }},
    { &hf_x420_next_available,
      { "next-available", "x420.next_available",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.Time", HFILL }},
    { &hf_x420_new_address,
      { "new-address", "x420.new_address",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ORDescriptor", HFILL }},
    { &hf_x420_effective_from,
      { "effective-from", "x420.effective_from",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.Time", HFILL }},
    { &hf_x420_assembly_instructions,
      { "assembly-instructions", "x420.assembly_instructions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.BodyPartReferences", HFILL }},
    { &hf_x420_BodyPartReferences_item,
      { "Item", "x420.BodyPartReferences_item",
        FT_UINT32, BASE_DEC, VALS(x420_BodyPartReference_vals), 0,
        "x420.BodyPartReference", HFILL }},
    { &hf_x420_stored_entry,
      { "stored-entry", "x420.stored_entry",
        FT_INT32, BASE_DEC, NULL, 0,
        "x420.SequenceNumber", HFILL }},
    { &hf_x420_stored_content,
      { "stored-content", "x420.stored_content",
        FT_INT32, BASE_DEC, NULL, 0,
        "x420.SequenceNumber", HFILL }},
    { &hf_x420_submitted_body_part,
      { "submitted-body-part", "x420.submitted_body_part",
        FT_INT32, BASE_DEC, NULL, 0,
        "x420.INTEGER", HFILL }},
    { &hf_x420_stored_body_part,
      { "stored-body-part", "x420.stored_body_part",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.T_stored_body_part", HFILL }},
    { &hf_x420_message_entry,
      { "message-entry", "x420.message_entry",
        FT_INT32, BASE_DEC, NULL, 0,
        "x420.SequenceNumber", HFILL }},
    { &hf_x420_body_part_number,
      { "body-part-number", "x420.body_part_number",
        FT_INT32, BASE_DEC, NULL, 0,
        "x420.BodyPartNumber", HFILL }},
    { &hf_x420_Languages_item,
      { "Item", "x420.Languages_item",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.Language", HFILL }},
    { &hf_x420_algorithmIdentifier,
      { "algorithmIdentifier", "x420.algorithmIdentifier",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509af.AlgorithmIdentifier", HFILL }},
    { &hf_x420_encrypted,
      { "encrypted", "x420.encrypted",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x420.BIT_STRING", HFILL }},
    { &hf_x420_BodyPartSignatures_item,
      { "Item", "x420.BodyPartSignatures_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.BodyPartSignatures_item", HFILL }},
    { &hf_x420_body_part_signature,
      { "body-part-signature", "x420.body_part_signature",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.BodyPartSignature", HFILL }},
    { &hf_x420_originator_certificate_selector,
      { "originator-certificate-selector", "x420.originator_certificate_selector",
        FT_NONE, BASE_NONE, NULL, 0,
        "x509ce.CertificateAssertion", HFILL }},
    { &hf_x420_content_security_label,
      { "content-security-label", "x420.content_security_label",
        FT_NONE, BASE_NONE, NULL, 0,
        "x411.SecurityLabel", HFILL }},
    { &hf_x420_heading_security_label,
      { "heading-security-label", "x420.heading_security_label",
        FT_NONE, BASE_NONE, NULL, 0,
        "x411.SecurityLabel", HFILL }},
    { &hf_x420_body_part_security_labels,
      { "body-part-security-labels", "x420.body_part_security_labels",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.SEQUENCE_OF_BodyPartSecurityLabel", HFILL }},
    { &hf_x420_body_part_security_labels_item,
      { "Item", "x420.body_part_security_labels_item",
        FT_UINT32, BASE_DEC, VALS(x420_BodyPartSecurityLabel_vals), 0,
        "x420.BodyPartSecurityLabel", HFILL }},
    { &hf_x420_body_part_unlabelled,
      { "body-part-unlabelled", "x420.body_part_unlabelled",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.NULL", HFILL }},
    { &hf_x420_body_part_security_label,
      { "body-part-security-label", "x420.body_part_security_label",
        FT_NONE, BASE_NONE, NULL, 0,
        "x411.SecurityLabel", HFILL }},
    { &hf_x420_CirculationList_item,
      { "Item", "x420.CirculationList_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.CirculationMember", HFILL }},
    { &hf_x420_circulation_recipient,
      { "circulation-recipient", "x420.circulation_recipient",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.RecipientSpecifier", HFILL }},
    { &hf_x420_checked,
      { "checked", "x420.checked",
        FT_UINT32, BASE_DEC, VALS(x420_Checkmark_vals), 0,
        "x420.Checkmark", HFILL }},
    { &hf_x420_simple,
      { "simple", "x420.simple",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.NULL", HFILL }},
    { &hf_x420_timestamped,
      { "timestamped", "x420.timestamped",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.CirculationTime", HFILL }},
    { &hf_x420_signed,
      { "signed", "x420.signed",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.CirculationSignature", HFILL }},
    { &hf_x420_circulation_signature_algorithm_identifier,
      { "algorithm-identifier", "x420.algorithm_identifier",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.CirculationSignatureAlgorithmIdentifier", HFILL }},
    { &hf_x420_timestamp,
      { "timestamp", "x420.timestamp",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.CirculationTime", HFILL }},
    { &hf_x420_circulation_signature_data,
      { "circulation-signature-data", "x420.circulation_signature_data",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.CirculationSignatureData", HFILL }},
    { &hf_x420_DistributionCodes_item,
      { "Item", "x420.DistributionCodes_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.DistributionCode", HFILL }},
    { &hf_x420_oid_code,
      { "oid-code", "x420.oid_code",
        FT_OID, BASE_NONE, NULL, 0,
        "x420.OBJECT_IDENTIFIER", HFILL }},
    { &hf_x420_alphanumeric_code,
      { "alphanumeric-code", "x420.alphanumeric_code",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.AlphaCode", HFILL }},
    { &hf_x420_or_descriptor,
      { "or-descriptor", "x420.or_descriptor",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ORDescriptor", HFILL }},
    { &hf_x420_InformationCategories_item,
      { "Item", "x420.InformationCategories_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.InformationCategory", HFILL }},
    { &hf_x420_reference,
      { "reference", "x420.reference",
        FT_OID, BASE_NONE, NULL, 0,
        "x420.OBJECT_IDENTIFIER", HFILL }},
    { &hf_x420_description,
      { "description", "x420.description",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.DescriptionString", HFILL }},
    { &hf_x420_ManualHandlingInstructions_item,
      { "Item", "x420.ManualHandlingInstructions_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.ManualHandlingInstruction", HFILL }},
    { &hf_x420_GeneralTextParameters_item,
      { "Item", "x420.GeneralTextParameters_item",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.CharacterSetRegistration", HFILL }},
    { &hf_x420_voice_message_duration,
      { "voice-message-duration", "x420.voice_message_duration",
        FT_INT32, BASE_DEC, NULL, 0,
        "x420.INTEGER", HFILL }},
    { &hf_x420_voice_encoding_type,
      { "voice-encoding-type", "x420.voice_encoding_type",
        FT_OID, BASE_NONE, NULL, 0,
        "x420.OBJECT_IDENTIFIER", HFILL }},
    { &hf_x420_supplementary_information,
      { "supplementary-information", "x420.supplementary_information",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.IA5String", HFILL }},
    { &hf_x420_mts_identifier,
      { "mts-identifier", "x420.mts_identifier",
        FT_NONE, BASE_NONE, NULL, 0,
        "x411.MessageDeliveryIdentifier", HFILL }},
    { &hf_x420_submission_proof,
      { "submission-proof", "x420.submission_proof",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.SubmissionProof", HFILL }},
    { &hf_x420_proof_of_submission,
      { "proof-of-submission", "x420.proof_of_submission",
        FT_NONE, BASE_NONE, NULL, 0,
        "x411.ProofOfSubmission", HFILL }},
    { &hf_x420_originating_MTA_certificate,
      { "originating-MTA-certificate", "x420.originating_MTA_certificate",
        FT_NONE, BASE_NONE, NULL, 0,
        "x411.OriginatingMTACertificate", HFILL }},
    { &hf_x420_message_submission_envelope,
      { "message-submission-envelope", "x420.message_submission_envelope",
        FT_NONE, BASE_NONE, NULL, 0,
        "x411.MessageSubmissionEnvelope", HFILL }},
    { &hf_x420_related_stored_file,
      { "related-stored-file", "x420.related_stored_file",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.RelatedStoredFile", HFILL }},
    { &hf_x420_contents_type,
      { "contents-type", "x420.contents_type",
        FT_UINT32, BASE_DEC, VALS(x420_Contents_Type_Attribute_vals), 0,
        "x420.ContentsTypeParameter", HFILL }},
    { &hf_x420_environment,
      { "environment", "x420.environment",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.EnvironmentParameter", HFILL }},
    { &hf_x420_compression,
      { "compression", "x420.compression",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.CompressionParameter", HFILL }},
    { &hf_x420_file_attributes,
      { "file-attributes", "x420.file_attributes",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.FileAttributes", HFILL }},
    { &hf_x420_FileTransferData_item,
      { "Item", "x420.FileTransferData_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "acse.EXTERNALt", HFILL }},
    { &hf_x420_RelatedStoredFile_item,
      { "Item", "x420.RelatedStoredFile_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.RelatedStoredFile_item", HFILL }},
    { &hf_x420_file_identifier,
      { "file-identifier", "x420.file_identifier",
        FT_UINT32, BASE_DEC, VALS(x420_FileIdentifier_vals), 0,
        "x420.FileIdentifier", HFILL }},
    { &hf_x420_relationship,
      { "relationship", "x420.relationship",
        FT_UINT32, BASE_DEC, VALS(x420_Relationship_vals), 0,
        "x420.Relationship", HFILL }},
    { &hf_x420_pathname_and_version,
      { "pathname-and-version", "x420.pathname_and_version",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.PathnameandVersion", HFILL }},
    { &hf_x420_cross_reference,
      { "cross-reference", "x420.cross_reference",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.CrossReference", HFILL }},
    { &hf_x420_pathname,
      { "pathname", "x420.pathname",
        FT_UINT32, BASE_DEC, VALS(x420_Pathname_Attribute_vals), 0,
        "x420.Pathname_Attribute", HFILL }},
    { &hf_x420_file_version,
      { "file-version", "x420.file_version",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.GraphicString", HFILL }},
    { &hf_x420_application_cross_reference,
      { "application-cross-reference", "x420.application_cross_reference",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x420.OCTET_STRING", HFILL }},
    { &hf_x420_message_reference,
      { "message-reference", "x420.message_reference",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.MessageReference", HFILL }},
    { &hf_x420_body_part_reference,
      { "body-part-reference", "x420.body_part_reference",
        FT_INT32, BASE_DEC, NULL, 0,
        "x420.INTEGER", HFILL }},
    { &hf_x420_user_relative_identifier_ref,
      { "user-relative-identifier", "x420.user_relative_identifier",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.PrintableString", HFILL }},
    { &hf_x420_explicit_relationship,
      { "explicit-relationship", "x420.explicit_relationship",
        FT_INT32, BASE_DEC, VALS(x420_ExplicitRelationship_vals), 0,
        "x420.ExplicitRelationship", HFILL }},
    { &hf_x420_descriptive_relationship,
      { "descriptive-relationship", "x420.descriptive_relationship",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.GraphicString", HFILL }},
    { &hf_x420_document_type,
      { "document-type", "x420.document_type",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.T_document_type", HFILL }},
    { &hf_x420_document_type_name,
      { "document-type-name", "x420.document_type_name",
        FT_OID, BASE_NONE, NULL, 0,
        "x420.Document_Type_Name", HFILL }},
    { &hf_x420_parameter,
      { "parameter", "x420.parameter",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.T_parameter", HFILL }},
    { &hf_x420_constraint_set_and_abstract_syntax,
      { "constraint-set-and-abstract-syntax", "x420.constraint_set_and_abstract_syntax",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.T_constraint_set_and_abstract_syntax", HFILL }},
    { &hf_x420_constraint_set_name,
      { "constraint-set-name", "x420.constraint_set_name",
        FT_OID, BASE_NONE, NULL, 0,
        "x420.Constraint_Set_Name", HFILL }},
    { &hf_x420_abstract_syntax_name,
      { "abstract-syntax-name", "x420.abstract_syntax_name",
        FT_OID, BASE_NONE, NULL, 0,
        "x420.Abstract_Syntax_Name", HFILL }},
    { &hf_x420_application_reference,
      { "application-reference", "x420.application_reference",
        FT_UINT32, BASE_DEC, VALS(x420_GeneralIdentifier_vals), 0,
        "x420.GeneralIdentifier", HFILL }},
    { &hf_x420_machine,
      { "machine", "x420.machine",
        FT_UINT32, BASE_DEC, VALS(x420_GeneralIdentifier_vals), 0,
        "x420.GeneralIdentifier", HFILL }},
    { &hf_x420_operating_system,
      { "operating-system", "x420.operating_system",
        FT_OID, BASE_NONE, NULL, 0,
        "x420.OBJECT_IDENTIFIER", HFILL }},
    { &hf_x420_user_visible_string,
      { "user-visible-string", "x420.user_visible_string",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.T_user_visible_string", HFILL }},
    { &hf_x420_user_visible_string_item,
      { "Item", "x420.user_visible_string_item",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.GraphicString", HFILL }},
    { &hf_x420_registered_identifier,
      { "registered-identifier", "x420.registered_identifier",
        FT_OID, BASE_NONE, NULL, 0,
        "x420.OBJECT_IDENTIFIER", HFILL }},
    { &hf_x420_descriptive_identifier,
      { "descriptive-identifier", "x420.descriptive_identifier",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.T_descriptive_identifier", HFILL }},
    { &hf_x420_descriptive_identifier_item,
      { "Item", "x420.descriptive_identifier_item",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.GraphicString", HFILL }},
    { &hf_x420_compression_algorithm_id,
      { "compression-algorithm-id", "x420.compression_algorithm_id",
        FT_OID, BASE_NONE, NULL, 0,
        "x420.OBJECT_IDENTIFIER", HFILL }},
    { &hf_x420_compression_algorithm_param,
      { "compression-algorithm-param", "x420.compression_algorithm_param",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.T_compression_algorithm_param", HFILL }},
    { &hf_x420_permitted_actions,
      { "permitted-actions", "x420.permitted_actions",
        FT_BYTES, BASE_HEX, NULL, 0,
        "ftam.Permitted_Actions_Attribute", HFILL }},
    { &hf_x420_storage_account,
      { "storage-account", "x420.storage_account",
        FT_UINT32, BASE_DEC, VALS(x420_Account_Attribute_vals), 0,
        "x420.Account_Attribute", HFILL }},
    { &hf_x420_date_and_time_of_creation,
      { "date-and-time-of-creation", "x420.date_and_time_of_creation",
        FT_UINT32, BASE_DEC, VALS(ftam_Date_and_Time_Attribute_vals), 0,
        "ftam.Date_and_Time_Attribute", HFILL }},
    { &hf_x420_date_and_time_of_last_modification,
      { "date-and-time-of-last-modification", "x420.date_and_time_of_last_modification",
        FT_UINT32, BASE_DEC, VALS(ftam_Date_and_Time_Attribute_vals), 0,
        "ftam.Date_and_Time_Attribute", HFILL }},
    { &hf_x420_date_and_time_of_last_read_access,
      { "date-and-time-of-last-read-access", "x420.date_and_time_of_last_read_access",
        FT_UINT32, BASE_DEC, VALS(ftam_Date_and_Time_Attribute_vals), 0,
        "ftam.Date_and_Time_Attribute", HFILL }},
    { &hf_x420_date_and_time_of_last_attribute_modification,
      { "date-and-time-of-last-attribute-modification", "x420.date_and_time_of_last_attribute_modification",
        FT_UINT32, BASE_DEC, VALS(ftam_Date_and_Time_Attribute_vals), 0,
        "ftam.Date_and_Time_Attribute", HFILL }},
    { &hf_x420_identity_of_creator,
      { "identity-of-creator", "x420.identity_of_creator",
        FT_UINT32, BASE_DEC, VALS(x420_User_Identity_Attribute_vals), 0,
        "x420.User_Identity_Attribute", HFILL }},
    { &hf_x420_identity_of_last_modifier,
      { "identity-of-last-modifier", "x420.identity_of_last_modifier",
        FT_UINT32, BASE_DEC, VALS(x420_User_Identity_Attribute_vals), 0,
        "x420.User_Identity_Attribute", HFILL }},
    { &hf_x420_identity_of_last_reader,
      { "identity-of-last-reader", "x420.identity_of_last_reader",
        FT_UINT32, BASE_DEC, VALS(x420_User_Identity_Attribute_vals), 0,
        "x420.User_Identity_Attribute", HFILL }},
    { &hf_x420_identity_of_last_attribute_modifier,
      { "identity-of-last-attribute-modifier", "x420.identity_of_last_attribute_modifier",
        FT_UINT32, BASE_DEC, VALS(x420_User_Identity_Attribute_vals), 0,
        "x420.User_Identity_Attribute", HFILL }},
    { &hf_x420_object_availability,
      { "object-availability", "x420.object_availability",
        FT_UINT32, BASE_DEC, VALS(ftam_Object_Availability_Attribute_vals), 0,
        "ftam.Object_Availability_Attribute", HFILL }},
    { &hf_x420_object_size,
      { "object-size", "x420.object_size",
        FT_UINT32, BASE_DEC, VALS(ftam_Object_Size_Attribute_vals), 0,
        "ftam.Object_Size_Attribute", HFILL }},
    { &hf_x420_future_object_size,
      { "future-object-size", "x420.future_object_size",
        FT_UINT32, BASE_DEC, VALS(ftam_Object_Size_Attribute_vals), 0,
        "ftam.Object_Size_Attribute", HFILL }},
    { &hf_x420_access_control,
      { "access-control", "x420.access_control",
        FT_UINT32, BASE_DEC, VALS(x420_Access_Control_Attribute_vals), 0,
        "x420.Access_Control_Attribute", HFILL }},
    { &hf_x420_legal_qualifications,
      { "legal-qualifications", "x420.legal_qualifications",
        FT_UINT32, BASE_DEC, VALS(ftam_Legal_Qualification_Attribute_vals), 0,
        "ftam.Legal_Qualification_Attribute", HFILL }},
    { &hf_x420_private_use,
      { "private-use", "x420.private_use",
        FT_UINT32, BASE_DEC, VALS(ftam_Private_Use_Attribute_vals), 0,
        "ftam.Private_Use_Attribute", HFILL }},
    { &hf_x420_attribute_extensions,
      { "attribute-extensions", "x420.attribute_extensions",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ftam.Attribute_Extensions", HFILL }},
    { &hf_x420_incomplete_pathname,
      { "incomplete-pathname", "x420.incomplete_pathname",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ftam.Pathname", HFILL }},
    { &hf_x420_complete_pathname,
      { "complete-pathname", "x420.complete_pathname",
        FT_UINT32, BASE_DEC, NULL, 0,
        "ftam.Pathname", HFILL }},
    { &hf_x420_no_value_available,
      { "no-value-available", "x420.no_value_available",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.NULL", HFILL }},
    { &hf_x420_account_actual_values,
      { "actual-values", "x420.actual_values",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.Account", HFILL }},
    { &hf_x420_identity_actual_values,
      { "actual-values", "x420.actual_values",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.User_Identity", HFILL }},
    { &hf_x420_actual_values,
      { "actual-values", "x420.actual_values",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.SET_OF_Access_Control_Element", HFILL }},
    { &hf_x420_actual_values_item,
      { "Item", "x420.actual_values_item",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.Access_Control_Element", HFILL }},
    { &hf_x420_action_list,
      { "action-list", "x420.action_list",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x420.Access_Request", HFILL }},
    { &hf_x420_concurrency_access,
      { "concurrency-access", "x420.concurrency_access",
        FT_NONE, BASE_NONE, NULL, 0,
        "ftam.Concurrency_Access", HFILL }},
    { &hf_x420_identity,
      { "identity", "x420.identity",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.User_Identity", HFILL }},
    { &hf_x420_passwords,
      { "passwords", "x420.passwords",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.Access_Passwords", HFILL }},
    { &hf_x420_location,
      { "location", "x420.location",
        FT_NONE, BASE_NONE, NULL, 0,
        "x420.Application_Entity_Title", HFILL }},
    { &hf_x420_read_password,
      { "read-password", "x420.read_password",
        FT_UINT32, BASE_DEC, VALS(x420_Password_vals), 0,
        "x420.Password", HFILL }},
    { &hf_x420_insert_password,
      { "insert-password", "x420.insert_password",
        FT_UINT32, BASE_DEC, VALS(x420_Password_vals), 0,
        "x420.Password", HFILL }},
    { &hf_x420_replace_password,
      { "replace-password", "x420.replace_password",
        FT_UINT32, BASE_DEC, VALS(x420_Password_vals), 0,
        "x420.Password", HFILL }},
    { &hf_x420_extend_password,
      { "extend-password", "x420.extend_password",
        FT_UINT32, BASE_DEC, VALS(x420_Password_vals), 0,
        "x420.Password", HFILL }},
    { &hf_x420_erase_password,
      { "erase-password", "x420.erase_password",
        FT_UINT32, BASE_DEC, VALS(x420_Password_vals), 0,
        "x420.Password", HFILL }},
    { &hf_x420_read_attribute_password,
      { "read-attribute-password", "x420.read_attribute_password",
        FT_UINT32, BASE_DEC, VALS(x420_Password_vals), 0,
        "x420.Password", HFILL }},
    { &hf_x420_change_attribute_password,
      { "change-attribute-password", "x420.change_attribute_password",
        FT_UINT32, BASE_DEC, VALS(x420_Password_vals), 0,
        "x420.Password", HFILL }},
    { &hf_x420_delete_password,
      { "delete-password", "x420.delete_password",
        FT_UINT32, BASE_DEC, VALS(x420_Password_vals), 0,
        "x420.Password", HFILL }},
    { &hf_x420_pass_passwords,
      { "pass-passwords", "x420.pass_passwords",
        FT_UINT32, BASE_DEC, NULL, 0,
        "x420.Pass_Passwords", HFILL }},
    { &hf_x420_link_password,
      { "link-password", "x420.link_password",
        FT_UINT32, BASE_DEC, VALS(x420_Password_vals), 0,
        "x420.Password", HFILL }},
    { &hf_x420_graphic_string,
      { "graphic-string", "x420.graphic_string",
        FT_STRING, BASE_NONE, NULL, 0,
        "x420.GraphicString", HFILL }},
    { &hf_x420_octet_string,
      { "octet-string", "x420.octet_string",
        FT_BYTES, BASE_HEX, NULL, 0,
        "x420.OCTET_STRING", HFILL }},
    { &hf_x420_Pass_Passwords_item,
      { "Item", "x420.Pass_Passwords_item",
        FT_UINT32, BASE_DEC, VALS(x420_Password_vals), 0,
        "x420.Password", HFILL }},
    { &hf_x420_ap_title,
      { "ap-title", "x420.ap_title",
        FT_UINT32, BASE_DEC, VALS(acse_AP_title_vals), 0,
        "acse.AP_title", HFILL }},
    { &hf_x420_ae_qualifier,
      { "ae-qualifier", "x420.ae_qualifier",
        FT_UINT32, BASE_DEC, VALS(acse_ASO_qualifier_vals), 0,
        "acse.AE_qualifier", HFILL }},
    { &hf_x420_NotificationRequests_rn,
      { "rn", "x420.rn",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_x420_NotificationRequests_nrn,
      { "nrn", "x420.nrn",
        FT_BOOLEAN, 8, NULL, 0x40,
        "", HFILL }},
    { &hf_x420_NotificationRequests_ipm_return,
      { "ipm-return", "x420.ipm-return",
        FT_BOOLEAN, 8, NULL, 0x20,
        "", HFILL }},
    { &hf_x420_NotificationRequests_an_supported,
      { "an-supported", "x420.an-supported",
        FT_BOOLEAN, 8, NULL, 0x10,
        "", HFILL }},
    { &hf_x420_NotificationRequests_suppress_an,
      { "suppress-an", "x420.suppress-an",
        FT_BOOLEAN, 8, NULL, 0x08,
        "", HFILL }},
    { &hf_x420_Access_Request_read,
      { "read", "x420.read",
        FT_BOOLEAN, 8, NULL, 0x80,
        "", HFILL }},
    { &hf_x420_Access_Request_insert,
      { "insert", "x420.insert",
        FT_BOOLEAN, 8, NULL, 0x40,
        "", HFILL }},
    { &hf_x420_Access_Request_replace,
      { "replace", "x420.replace",
        FT_BOOLEAN, 8, NULL, 0x20,
        "", HFILL }},
    { &hf_x420_Access_Request_extend,
      { "extend", "x420.extend",
        FT_BOOLEAN, 8, NULL, 0x10,
        "", HFILL }},
    { &hf_x420_Access_Request_erase,
      { "erase", "x420.erase",
        FT_BOOLEAN, 8, NULL, 0x08,
        "", HFILL }},
    { &hf_x420_Access_Request_read_attribute,
      { "read-attribute", "x420.read-attribute",
        FT_BOOLEAN, 8, NULL, 0x04,
        "", HFILL }},
    { &hf_x420_Access_Request_change_attribute,
      { "change-attribute", "x420.change-attribute",
        FT_BOOLEAN, 8, NULL, 0x02,
        "", HFILL }},
    { &hf_x420_Access_Request_delete_object,
      { "delete-object", "x420.delete-object",
        FT_BOOLEAN, 8, NULL, 0x01,
        "", HFILL }},

/*--- End of included file: packet-x420-hfarr.c ---*/
#line 129 "packet-x420-template.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
    &ett_x420,

/*--- Included file: packet-x420-ettarr.c ---*/
#line 1 "packet-x420-ettarr.c"
    &ett_x420_InformationObject,
    &ett_x420_IPM,
    &ett_x420_IPMSExtension,
    &ett_x420_Heading,
    &ett_x420_IPMIdentifier,
    &ett_x420_RecipientSpecifier,
    &ett_x420_ORDescriptor,
    &ett_x420_NotificationRequests,
    &ett_x420_RecipientExtensionsField,
    &ett_x420_AuthorizingUsersField,
    &ett_x420_PrimaryRecipientsField,
    &ett_x420_CopyRecipientsField,
    &ett_x420_BlindCopyRecipientsField,
    &ett_x420_ObsoletedIPMsField,
    &ett_x420_RelatedIPMsField,
    &ett_x420_ReplyRecipientsField,
    &ett_x420_ExtensionsField,
    &ett_x420_Body,
    &ett_x420_BodyPart,
    &ett_x420_ExtendedBodyPart,
    &ett_x420_IA5TextBodyPart,
    &ett_x420_IA5TextParameters,
    &ett_x420_G3FacsimileBodyPart,
    &ett_x420_G3FacsimileParameters,
    &ett_x420_G3FacsimileData,
    &ett_x420_G4Class1Data,
    &ett_x420_MixedModeData,
    &ett_x420_TeletexBodyPart,
    &ett_x420_TeletexParameters,
    &ett_x420_TeletexData,
    &ett_x420_VideotexBodyPart,
    &ett_x420_VideotexParameters,
    &ett_x420_EncryptedBodyPart,
    &ett_x420_EncryptedParameters,
    &ett_x420_MessageBodyPart,
    &ett_x420_MessageParameters,
    &ett_x420_IPN,
    &ett_x420_T_choice,
    &ett_x420_NonReceiptFields,
    &ett_x420_ReceiptFields,
    &ett_x420_NotificationExtensionsField,
    &ett_x420_NRNExtensionsField,
    &ett_x420_RNExtensionsField,
    &ett_x420_OtherNotificationTypeFields,
    &ett_x420_AbsenceAdvice,
    &ett_x420_ChangeOfAddressAdvice,
    &ett_x420_IPMAssemblyInstructions,
    &ett_x420_BodyPartReferences,
    &ett_x420_BodyPartReference,
    &ett_x420_T_stored_body_part,
    &ett_x420_Languages,
    &ett_x420_Signature,
    &ett_x420_BodyPartSignatures,
    &ett_x420_BodyPartSignatures_item,
    &ett_x420_IPMSecurityLabel,
    &ett_x420_SEQUENCE_OF_BodyPartSecurityLabel,
    &ett_x420_BodyPartSecurityLabel,
    &ett_x420_CirculationList,
    &ett_x420_CirculationMember,
    &ett_x420_Checkmark,
    &ett_x420_CirculationSignatureData,
    &ett_x420_CirculationSignature,
    &ett_x420_DistributionCodes,
    &ett_x420_DistributionCode,
    &ett_x420_InformationCategories,
    &ett_x420_InformationCategory,
    &ett_x420_ManualHandlingInstructions,
    &ett_x420_GeneralTextParameters,
    &ett_x420_VoiceParameters,
    &ett_x420_ForwardedContentParameters,
    &ett_x420_SubmissionProof,
    &ett_x420_FileTransferParameters,
    &ett_x420_FileTransferData,
    &ett_x420_RelatedStoredFile,
    &ett_x420_RelatedStoredFile_item,
    &ett_x420_FileIdentifier,
    &ett_x420_PathnameandVersion,
    &ett_x420_CrossReference,
    &ett_x420_MessageReference,
    &ett_x420_Relationship,
    &ett_x420_Contents_Type_Attribute,
    &ett_x420_T_document_type,
    &ett_x420_T_constraint_set_and_abstract_syntax,
    &ett_x420_EnvironmentParameter,
    &ett_x420_T_user_visible_string,
    &ett_x420_GeneralIdentifier,
    &ett_x420_T_descriptive_identifier,
    &ett_x420_CompressionParameter,
    &ett_x420_FileAttributes,
    &ett_x420_Pathname_Attribute,
    &ett_x420_Account_Attribute,
    &ett_x420_User_Identity_Attribute,
    &ett_x420_Access_Control_Attribute,
    &ett_x420_SET_OF_Access_Control_Element,
    &ett_x420_Access_Control_Element,
    &ett_x420_Access_Request,
    &ett_x420_Access_Passwords,
    &ett_x420_Password,
    &ett_x420_Pass_Passwords,
    &ett_x420_Application_Entity_Title,

/*--- End of included file: packet-x420-ettarr.c ---*/
#line 135 "packet-x420-template.c"
  };

  /* Register protocol */
  proto_x420 = proto_register_protocol(PNAME, PSNAME, PFNAME);
  register_dissector("x420", dissect_x420, proto_x420);
  /* Register fields and subtrees */
  proto_register_field_array(proto_x420, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));

}


/*--- proto_reg_handoff_x420 --- */
void proto_reg_handoff_x420(void) {


/*--- Included file: packet-x420-dis-tab.c ---*/
#line 1 "packet-x420-dis-tab.c"
  register_ber_oid_dissector("1.2.826.0.1004.10.1.1", dissect_OriginatingUA_PDU, proto_x420, "nexor-originating-ua");
  register_ber_oid_dissector("2.6.1.19.0", dissect_AbsenceAdvice_PDU, proto_x420, "id-on-absence-advice");
  register_ber_oid_dissector("2.6.1.19.1", dissect_ChangeOfAddressAdvice_PDU, proto_x420, "id-on-change-of-address-advice");
  register_ber_oid_dissector("2.6.1.17.2", dissect_IPMAssemblyInstructions_PDU, proto_x420, "id-mst-assembly-instructions");
  register_ber_oid_dissector("2.6.1.5.0", dissect_IncompleteCopy_PDU, proto_x420, "id-hex-incomplete-copy");
  register_ber_oid_dissector("2.6.1.5.1", dissect_Languages_PDU, proto_x420, "id-hex-languages");
  register_ber_oid_dissector("2.6.1.5.2", dissect_AutoSubmitted_PDU, proto_x420, "id-hex-auto-submitted");
  register_ber_oid_dissector("2.6.1.5.3", dissect_BodyPartSignatures_PDU, proto_x420, "id-hex-body-part-signatures");
  register_ber_oid_dissector("2.6.1.5.4", dissect_IPMSecurityLabel_PDU, proto_x420, "id-hex-ipm-security-label");
  register_ber_oid_dissector("2.6.1.5.5", dissect_AuthorizationTime_PDU, proto_x420, "id-hex-authorization-time");
  register_ber_oid_dissector("2.6.1.5.6", dissect_CirculationList_PDU, proto_x420, "id-hex-circulation-list-recipients");
  register_ber_oid_dissector("2.6.1.20.0", dissect_CirculationListIndicator_PDU, proto_x420, "id-rex-circulation-list-indicator");
  register_ber_oid_dissector("2.6.1.5.7", dissect_DistributionCodes_PDU, proto_x420, "id-hex-distribution-codes");
  register_ber_oid_dissector("2.6.1.5.8", dissect_ExtendedSubject_PDU, proto_x420, "id-hex-extended-subject");
  register_ber_oid_dissector("2.6.1.5.9", dissect_InformationCategories_PDU, proto_x420, "id-hex-information-categories");
  register_ber_oid_dissector("2.6.1.5.10", dissect_ManualHandlingInstructions_PDU, proto_x420, "id-hex-manual-handling-instructions");
  register_ber_oid_dissector("2.6.1.5.11", dissect_OriginatorsReference_PDU, proto_x420, "id-hex-originators-reference");
  register_ber_oid_dissector("2.6.1.5.12", dissect_PrecedencePolicyIdentifier_PDU, proto_x420, "id-hex-precedence-policy-id");
  register_ber_oid_dissector("2.6.1.20.1", dissect_Precedence_PDU, proto_x420, "id-rex-precedence");
  register_ber_oid_dissector("2.6.1.4.0", dissect_IA5TextData_PDU, proto_x420, "id-et-ia5-text");
  register_ber_oid_dissector("2.6.1.11.0", dissect_IA5TextParameters_PDU, proto_x420, "id-ep-ia5-text");
  register_ber_oid_dissector("2.6.1.4.2", dissect_G3FacsimileData_PDU, proto_x420, "id-et-g3-facsimile");
  register_ber_oid_dissector("2.6.1.11.2", dissect_G3FacsimileParameters_PDU, proto_x420, "id-ep-g3-facsimile");
  register_ber_oid_dissector("2.6.1.4.3", dissect_G4Class1Data_PDU, proto_x420, "id-et-g4-class1");
  register_ber_oid_dissector("2.6.1.4.4", dissect_TeletexData_PDU, proto_x420, "id-et-teletex");
  register_ber_oid_dissector("2.6.1.11.4", dissect_TeletexParameters_PDU, proto_x420, "id-ep-teletex");
  register_ber_oid_dissector("2.6.1.4.5", dissect_VideotexData_PDU, proto_x420, "id-et-videotex");
  register_ber_oid_dissector("2.6.1.11.5", dissect_VideotexParameters_PDU, proto_x420, "id-ep-videotex");
  register_ber_oid_dissector("2.6.1.4.6", dissect_EncryptedData_PDU, proto_x420, "id-et-encrypted");
  register_ber_oid_dissector("2.6.1.11.6", dissect_EncryptedParameters_PDU, proto_x420, "id-ep-encrypted");
  register_ber_oid_dissector("2.6.1.4.7", dissect_MessageData_PDU, proto_x420, "id-et-message");
  register_ber_oid_dissector("2.6.1.11.7", dissect_MessageParameters_PDU, proto_x420, "id-ep-message");
  register_ber_oid_dissector("2.6.1.4.8", dissect_MixedModeData_PDU, proto_x420, "id-et-mixed-mode");
  register_ber_oid_dissector("2.6.1.4.9", dissect_BilaterallyDefinedBodyPart_PDU, proto_x420, "id-et-bilaterally-defined");
  register_ber_oid_dissector("2.6.1.11.11", dissect_GeneralTextParameters_PDU, proto_x420, "id-ep-general-text");
  register_ber_oid_dissector("2.6.1.4.11", dissect_GeneralTextData_PDU, proto_x420, "id-et-general-text");
  register_ber_oid_dissector("2.6.1.11.12", dissect_FileTransferParameters_PDU, proto_x420, "id-ep-file-transfer");
  register_ber_oid_dissector("2.6.1.4.12", dissect_FileTransferData_PDU, proto_x420, "id-et-file-transfer");
  register_ber_oid_dissector("2.6.1.11.15", dissect_MessageParameters_PDU, proto_x420, "id-ep-notification");
  register_ber_oid_dissector("2.6.1.4.15", dissect_IPN_PDU, proto_x420, "id-et-notification");
  register_ber_oid_dissector("2.6.1.11.16", dissect_VoiceParameters_PDU, proto_x420, "id-ep-voice");
  register_ber_oid_dissector("2.6.1.4.16", dissect_VoiceData_PDU, proto_x420, "id-et-voice");
  register_ber_oid_dissector("2.6.1.11.17.2.6.1.10.1", dissect_ForwardedContentParameters_PDU, proto_x420, "id-ep-content-p22");
  register_ber_oid_dissector("2.6.1.4.17.2.6.1.10.1", dissect_InformationObject_PDU, proto_x420, "id-et-content-p22");
  register_ber_oid_dissector("2.6.1.11.17.2.6.1.10.0", dissect_ForwardedContentParameters_PDU, proto_x420, "id-ep-content-p2");
  register_ber_oid_dissector("2.6.1.4.17.2.6.1.10.0", dissect_InformationObject_PDU, proto_x420, "id-et-content-p2");
  register_ber_oid_dissector("2.6.1.11.17.1.3.26.0.4406.0.4.1", dissect_ForwardedContentParameters_PDU, proto_x420, "id-ep-content-p772");


/*--- End of included file: packet-x420-dis-tab.c ---*/
#line 151 "packet-x420-template.c"

  register_ber_oid_dissector("2.6.1.10.0", dissect_x420, proto_x420, "InterPersonal Message (1984)");
  register_ber_oid_dissector("2.6.1.10.1", dissect_x420, proto_x420, "InterPersonal Message (1988)");


}
