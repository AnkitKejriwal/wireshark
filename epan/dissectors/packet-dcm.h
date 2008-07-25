/* packet-dcm.h
 *
 * $Id$
 * Routines for DICOM packet dissection
 * Copyright 2008, David Aggeler <david_aggeler@hispeed.ch>
 * This file is generated automatically from part 6 of the standard
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

#ifndef PACKET_DCM_H
#define PACKET_DCM_H

/* Used for DICOM Export Object feature */
typedef struct _dicom_eo_t {
	guint32  pkt_num;
	gchar   *hostname;
	gchar   *filename;
	gchar   *content_type;
	guint32  payload_len;
	const guint8 *payload_data;
} dicom_eo_t;

/* --------------------------------------------------------------------- 
 * DICOM UID Definitions

 * Part 6 lists following different UID Types (2006-2008)

 * Application Context Name
 * Coding Scheme
 * DICOM UIDs as a Coding Scheme
 * LDAP OID
 * Meta SOP Class
 * SOP Class
 * Service Class
 * Transfer Syntax
 * Well-known Print Queue SOP Instance
 * Well-known Printer SOP Instance
 * Well-known SOP Instance
 * Well-known frame of reference
 */
 
struct dcm_uid {
    const char *value;
    const char *name;
    const char *type;
};

typedef struct dcm_uid dcm_uid_t;

static GHashTable *dcm_uid_table = NULL;

static dcm_uid_t dcm_uid_data[] = {
    { "1.2.840.10008.1.1", "Verification SOP Class", "SOP Class"},
    { "1.2.840.10008.1.2", "Implicit VR Little Endian", "Transfer Syntax"},
    { "1.2.840.10008.1.2.1", "Explicit VR Little Endian", "Transfer Syntax"},
    { "1.2.840.10008.1.2.1.99", "Deflated Explicit VR Little Endian", "Transfer Syntax"},
    { "1.2.840.10008.1.2.2", "Explicit VR Big Endian", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.50", "JPEG Baseline (Process 1): Default Transfer Syntax for Lossy JPEG 8 Bit Image Compression", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.51", "JPEG Extended (Process 2 & 4): Default Transfer Syntax for Lossy JPEG 12 Bit Image Compression (Process 4 only)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.52", "JPEG Extended (Process 3 & 5) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.53", "JPEG Spectral Selection, Non-Hierarchical (Process 6 & 8) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.54", "JPEG Spectral Selection, Non-Hierarchical (Process 7 & 9) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.55", "JPEG Full Progression, Non-Hierarchical (Process 10 & 12) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.56", "JPEG Full Progression, Non-Hierarchical (Process 11 & 13) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.57", "JPEG Lossless, Non-Hierarchical (Process 14)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.58", "JPEG Lossless, Non-Hierarchical (Process 15) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.59", "JPEG Extended, Hierarchical (Process 16 & 18) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.60", "JPEG Extended, Hierarchical (Process 17 & 19) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.61", "JPEG Spectral Selection, Hierarchical (Process 20 & 22) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.62", "JPEG Spectral Selection, Hierarchical (Process 21 & 23) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.63", "JPEG Full Progression, Hierarchical (Process 24 & 26) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.64", "JPEG Full Progression, Hierarchical (Process 25 & 27) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.65", "JPEG Lossless, Hierarchical (Process 28) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.66", "JPEG Lossless, Hierarchical (Process 29) (Retired)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.70", "JPEG Lossless, Non-Hierarchical, First-Order Prediction (Process 14 [Selection Value 1]): Default Transfer Syntax for Lossless JPEG Image Compression", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.80", "JPEG-LS Lossless Image Compression", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.81", "JPEG-LS Lossy (Near-Lossless) Image Compression", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.90", "JPEG 2000 Image Compression (Lossless Only)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.91", "JPEG 2000 Image Compression", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.92", "JPEG 2000 Part 2 Multi-component Image Compression (Lossless Only)", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.93", "JPEG 2000 Part 2 Multi-component Image Compression", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.94", "JPIP Referenced", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.95", "JPIP Referenced Deflate", "Transfer Syntax"},
    { "1.2.840.10008.1.2.4.100", "MPEG2 Main Profile @ Main Level", "Transfer Syntax"},
    { "1.2.840.10008.1.2.5", "RLE Lossless", "Transfer Syntax"},
    { "1.2.840.10008.1.2.6.1", "RFC 2557 MIME encapsulation", "Transfer Syntax"},
    { "1.2.840.10008.1.2.6.2", "XML Encoding", "Transfer Syntax"},
    { "1.2.840.10008.1.3.10", "Media Storage Directory Storage", "SOP Class"},
    { "1.2.840.10008.1.4.1.1", "Talairach Brain Atlas Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.2", "SPM2 T1 Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.3", "SPM2 T2 Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.4", "SPM2 PD Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.5", "SPM2 EPI Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.6", "SPM2 FIL T1 Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.7", "SPM2 PET Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.8", "SPM2 TRANSM Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.9", "SPM2 SPECT Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.10", "SPM2 GRAY Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.11", "SPM2 WHITE Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.12", "SPM2 CSF Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.13", "SPM2 BRAINMASK Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.14", "SPM2 AVG305T1 Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.15", "SPM2 AVG152T1 Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.16", "SPM2 AVG152T2 Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.17", "SPM2 AVG152PD Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.1.18", "SPM2 SINGLESUBJT1 Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.2.1", "ICBM 452 T1 Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.4.2.2", "ICBM Single Subject MRI Frame of Reference", "Well-known frame of reference"},
    { "1.2.840.10008.1.9", "Basic Study Content Notification SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.1.20.1", "Storage Commitment Push Model SOP Class", "SOP Class"},
    { "1.2.840.10008.1.20.1.1", "Storage Commitment Push Model SOP Instance", "Well-known SOP Instance"},
    { "1.2.840.10008.1.20.2", "Storage Commitment Pull Model SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.1.20.2.1", "Storage Commitment Pull Model SOP Instance (Retired)", "Well-known SOP Instance"},
    { "1.2.840.10008.1.40", "Procedural Event Logging SOP Class", "SOP Class"},
    { "1.2.840.10008.1.40.1", "Procedural Event Logging SOP Instance", "Well-known SOP Instance"},
    { "1.2.840.10008.1.42", "Substance Administration Logging SOP Class", "SOP Class"},
    { "1.2.840.10008.1.42.1", "Substance Administration Logging SOP Instance", "Well-known SOP Instance"},
    { "1.2.840.10008.2.6.1", "DICOM UID Registry", "DICOM UIDs as a Coding Scheme"},
    { "1.2.840.10008.2.16.4", "DICOM Controlled Terminology", "Coding Scheme"},
    { "1.2.840.10008.3.1.1.1", "DICOM Application Context Name", "Application Context Name"},
    { "1.2.840.10008.3.1.2.1.1", "Detached Patient Management SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.3.1.2.1.4", "Detached Patient Management Meta SOP Class (Retired)", "Meta SOP Class"},
    { "1.2.840.10008.3.1.2.2.1", "Detached Visit Management SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.3.1.2.3.1", "Detached Study Management SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.3.1.2.3.2", "Study Component Management SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.3.1.2.3.3", "Modality Performed Procedure Step SOP Class", "SOP Class"},
    { "1.2.840.10008.3.1.2.3.4", "Modality Performed Procedure Step Retrieve SOP Class", "SOP Class"},
    { "1.2.840.10008.3.1.2.3.5", "Modality Performed Procedure Step Notification SOP Class", "SOP Class"},
    { "1.2.840.10008.3.1.2.5.1", "Detached Results Management SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.3.1.2.5.4", "Detached Results Management Meta SOP Class (Retired)", "Meta SOP Class"},
    { "1.2.840.10008.3.1.2.5.5", "Detached Study Management Meta SOP Class (Retired)", "Meta SOP Class"},
    { "1.2.840.10008.3.1.2.6.1", "Detached Interpretation Management SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.4.2", "Storage Service Class", "Service Class"},
    { "1.2.840.10008.5.1.1.1", "Basic Film Session SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.1.2", "Basic Film Box SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.1.4", "Basic Grayscale Image Box SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.1.4.1", "Basic Color Image Box SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.1.4.2", "Referenced Image Box SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.1.9", "Basic Grayscale Print Management Meta SOP Class", "Meta SOP Class"},
    { "1.2.840.10008.5.1.1.9.1", "Referenced Grayscale Print Management Meta SOP Class (Retired)", "Meta SOP Class"},
    { "1.2.840.10008.5.1.1.14", "Print Job SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.1.15", "Basic Annotation Box SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.1.16", "Printer SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.1.16.376", "Printer Configuration Retrieval SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.1.17", "Printer SOP Instance", "Well-known Printer SOP Instance"},
    { "1.2.840.10008.5.1.1.17.376", "Printer Configuration Retrieval SOP Instance", "Well-known Printer SOP Instance"},
    { "1.2.840.10008.5.1.1.18", "Basic Color Print Management Meta SOP Class", "Meta SOP Class"},
    { "1.2.840.10008.5.1.1.18.1", "Referenced Color Print Management Meta SOP Class (Retired)", "Meta SOP Class"},
    { "1.2.840.10008.5.1.1.22", "VOI LUT Box SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.1.23", "Presentation LUT SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.1.24", "Image Overlay Box SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.1.24.1", "Basic Print Image Overlay Box SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.1.25", "Print Queue SOP Instance (Retired)", "Well-known Print Queue SOP Instance"},
    { "1.2.840.10008.5.1.1.26", "Print Queue Management SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.1.27", "Stored Print Storage SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.1.29", "Hardcopy Grayscale Image Storage SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.1.30", "Hardcopy Color Image Storage SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.1.31", "Pull Print Request SOP Class (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.1.32", "Pull Stored Print Management Meta SOP Class (Retired)", "Meta SOP Class"},
    { "1.2.840.10008.5.1.1.33", "Media Creation Management SOP Class UID", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.1", "Computed Radiography Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.1.1", "Digital X-Ray Image Storage - For Presentation", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.1.1.1", "Digital X-Ray Image Storage - For Processing", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.1.2", "Digital Mammography X-Ray Image Storage - For Presentation", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.1.2.1", "Digital Mammography X-Ray Image Storage - For Processing", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.1.3", "Digital Intra-oral X-Ray Image Storage - For Presentation", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.1.3.1", "Digital Intra-oral X-Ray Image Storage - For Processing", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.2", "CT Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.2.1", "Enhanced CT Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.3", "Ultrasound Multi-frame Image Storage (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.3.1", "Ultrasound Multi-frame Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.4", "MR Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.4.1", "Enhanced MR Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.4.2", "MR Spectroscopy Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.5", "Nuclear Medicine Image Storage (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.6", "Ultrasound Image Storage (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.6.1", "Ultrasound Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.7", "Secondary Capture Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.7.1", "Multi-frame Single Bit Secondary Capture Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.7.2", "Multi-frame Grayscale Byte Secondary Capture Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.7.3", "Multi-frame Grayscale Word Secondary Capture Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.7.4", "Multi-frame True Color Secondary Capture Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.8", "Standalone Overlay Storage (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.9", "Standalone Curve Storage (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.9.1", "Waveform Storage - Trial (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.9.1.1", "12-lead ECG Waveform Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.9.1.2", "General ECG Waveform Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.9.1.3", "Ambulatory ECG Waveform Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.9.2.1", "Hemodynamic Waveform Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.9.3.1", "Cardiac Electrophysiology Waveform Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.9.4.1", "Basic Voice Audio Waveform Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.10", "Standalone Modality LUT Storage (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.11", "Standalone VOI LUT Storage (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.11.1", "Grayscale Softcopy Presentation State Storage SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.11.2", "Color Softcopy Presentation State Storage SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.11.3", "Pseudo-Color Softcopy Presentation State Storage SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.11.4", "Blending Softcopy Presentation State Storage SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.12.1", "X-Ray Angiographic Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.12.1.1", "Enhanced XA Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.12.2", "X-Ray Radiofluoroscopic Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.12.2.1", "Enhanced XRF Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.13.1.1", "X-Ray 3D Angiographic Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.13.1.2", "X-Ray 3D Craniofacial Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.12.3", "X-Ray Angiographic Bi-Plane Image Storage (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.20", "Nuclear Medicine Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.66", "Raw Data Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.66.1", "Spatial Registration Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.66.2", "Spatial Fiducials Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.66.3", "Deformable Spatial Registration Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.66.4", "Segmentation Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.67", "Real World Value Mapping Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.77.1", "VL Image Storage - Trial (Retired)", ""},
    { "1.2.840.10008.5.1.4.1.1.77.2", "VL Multi-frame Image Storage - Trial (Retired)", ""},
    { "1.2.840.10008.5.1.4.1.1.77.1.1", "VL Endoscopic Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.77.1.1.1", "Video Endoscopic Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.77.1.2", "VL Microscopic Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.77.1.2.1", "Video Microscopic Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.77.1.3", "VL Slide-Coordinates Microscopic Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.77.1.4", "VL Photographic Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.77.1.4.1", "Video Photographic Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.77.1.5.1", "Ophthalmic Photography 8 Bit Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.77.1.5.2", "Ophthalmic Photography 16 Bit Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.77.1.5.3", "Stereometric Relationship Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.77.1.5.4", "Ophthalmic Tomography Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.1", "Text SR Storage - Trial (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.2", "Audio SR Storage - Trial (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.3", "Detail SR Storage - Trial (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.4", "Comprehensive SR Storage - Trial (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.11", "Basic Text SR Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.22", "Enhanced SR Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.33", "Comprehensive SR Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.40", "Procedure Log Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.50", "Mammography CAD SR Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.59", "Key Object Selection Document Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.65", "Chest CAD SR Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.88.67", "X-Ray Radiation Dose SR Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.104.1", "Encapsulated PDF Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.104.2", "Encapsulated CDA Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.128", "Positron Emission Tomography Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.129", "Standalone PET Curve Storage (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.481.1", "RT Image Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.481.2", "RT Dose Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.481.3", "RT Structure Set Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.481.4", "RT Beams Treatment Record Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.481.5", "RT Plan Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.481.6", "RT Brachy Treatment Record Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.481.7", "RT Treatment Summary Record Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.481.8", "RT Ion Plan Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.1.481.9", "RT Ion Beams Treatment Record Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.2.1.1", "Patient Root Query/Retrieve Information Model - FIND", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.2.1.2", "Patient Root Query/Retrieve Information Model - MOVE", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.2.1.3", "Patient Root Query/Retrieve Information Model - GET", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.2.2.1", "Study Root Query/Retrieve Information Model - FIND", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.2.2.2", "Study Root Query/Retrieve Information Model - MOVE", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.2.2.3", "Study Root Query/Retrieve Information Model - GET", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.2.3.1", "Patient/Study Only Query/Retrieve Information Model - FIND (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.2.3.2", "Patient/Study Only Query/Retrieve Information Model - MOVE (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.1.2.3.3", "Patient/Study Only Query/Retrieve Information Model - GET (Retired)", "SOP Class"},
    { "1.2.840.10008.5.1.4.31", "Modality Worklist Information Model - FIND", "SOP Class"},
    { "1.2.840.10008.5.1.4.32.1", "General Purpose Worklist Information Model - FIND", "SOP Class"},
    { "1.2.840.10008.5.1.4.32.2", "General Purpose Scheduled Procedure Step SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.32.3", "General Purpose Performed Procedure Step SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.32", "General Purpose Worklist Management Meta SOP Class", "Meta SOP Class"},
    { "1.2.840.10008.5.1.4.33", "Instance Availability Notification SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.34.1", "RT Beams Delivery Instruction Storage (Supplement 74 Frozen Draft)", "SOP Class"},
    { "1.2.840.10008.5.1.4.34.2", "RT Conventional Machine Verification (Supplement 74 Frozen Draft)", "SOP Class"},
    { "1.2.840.10008.5.1.4.34.3", "RT Ion Machine Verification (Supplement 74 Frozen Draft)", "SOP Class"},
    { "1.2.840.10008.5.1.4.34.4", "Unified Worklist and Procedure Step Service Class", "Service Class"},
    { "1.2.840.10008.5.1.4.34.4.1", "Unified Procedure Step - Push SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.34.4.2", "Unified Procedure Step - Watch SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.34.4.3", "Unified Procedure Step - Pull SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.34.4.4", "Unified Procedure Step - Event SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.34.5", "Unified Worklist and Procedure Step SOP Instance", "Well-known SOP Instance"},
    { "1.2.840.10008.5.1.4.37.1", "General Relevant Patient Information Query", "SOP Class"},
    { "1.2.840.10008.5.1.4.37.2", "Breast Imaging Relevant Patient Information Query", "SOP Class"},
    { "1.2.840.10008.5.1.4.37.3", "Cardiac Relevant Patient Information Query", "SOP Class"},
    { "1.2.840.10008.5.1.4.38.1", "Hanging Protocol Storage", "SOP Class"},
    { "1.2.840.10008.5.1.4.38.2", "Hanging Protocol Information Model - FIND", "SOP Class"},
    { "1.2.840.10008.5.1.4.38.3", "Hanging Protocol Information Model - MOVE", "SOP Class"},
    { "1.2.840.10008.5.1.4.41", "Product Characteristics Query SOP Class", "SOP Class"},
    { "1.2.840.10008.5.1.4.42", "Substance Approval Query SOP Class", "SOP Class"},
    { "1.2.840.10008.15.0.3.1", "dicomDeviceName", "LDAP OID"},
    { "1.2.840.10008.15.0.3.2", "dicomDescription", "LDAP OID"},
    { "1.2.840.10008.15.0.3.3", "dicomManufacturer", "LDAP OID"},
    { "1.2.840.10008.15.0.3.4", "dicomManufacturerModelName", "LDAP OID"},
    { "1.2.840.10008.15.0.3.5", "dicomSoftwareVersion", "LDAP OID"},
    { "1.2.840.10008.15.0.3.6", "dicomVendorData", "LDAP OID"},
    { "1.2.840.10008.15.0.3.7", "dicomAETitle", "LDAP OID"},
    { "1.2.840.10008.15.0.3.8", "dicomNetworkConnectionReference", "LDAP OID"},
    { "1.2.840.10008.15.0.3.9", "dicomApplicationCluster", "LDAP OID"},
    { "1.2.840.10008.15.0.3.10", "dicomAssociationInitiator", "LDAP OID"},
    { "1.2.840.10008.15.0.3.11", "dicomAssociationAcceptor", "LDAP OID"},
    { "1.2.840.10008.15.0.3.12", "dicomHostname", "LDAP OID"},
    { "1.2.840.10008.15.0.3.13", "dicomPort", "LDAP OID"},
    { "1.2.840.10008.15.0.3.14", "dicomSOPClass", "LDAP OID"},
    { "1.2.840.10008.15.0.3.15", "dicomTransferRole", "LDAP OID"},
    { "1.2.840.10008.15.0.3.16", "dicomTransferSyntax", "LDAP OID"},
    { "1.2.840.10008.15.0.3.17", "dicomPrimaryDeviceType", "LDAP OID"},
    { "1.2.840.10008.15.0.3.18", "dicomRelatedDeviceReference", "LDAP OID"},
    { "1.2.840.10008.15.0.3.19", "dicomPreferredCalledAETitle", "LDAP OID"},
    { "1.2.840.10008.15.0.3.20", "dicomTLSCyphersuite", "LDAP OID"},
    { "1.2.840.10008.15.0.3.21", "dicomAuthorizedNodeCertificateReference", "LDAP OID"},
    { "1.2.840.10008.15.0.3.22", "dicomThisNodeCertificateReference", "LDAP OID"},
    { "1.2.840.10008.15.0.3.23", "dicomInstalled", "LDAP OID"},
    { "1.2.840.10008.15.0.3.24", "dicomStationName", "LDAP OID"},
    { "1.2.840.10008.15.0.3.25", "dicomDeviceSerialNumber", "LDAP OID"},
    { "1.2.840.10008.15.0.3.26", "dicomInstitutionName", "LDAP OID"},
    { "1.2.840.10008.15.0.3.27", "dicomInstitutionAddress", "LDAP OID"},
    { "1.2.840.10008.15.0.3.28", "dicomInstitutionDepartmentName", "LDAP OID"},
    { "1.2.840.10008.15.0.3.29", "dicomIssuerOfPatientID", "LDAP OID"},
    { "1.2.840.10008.15.0.3.30", "dicomPreferredCallingAETitle", "LDAP OID"},
    { "1.2.840.10008.15.0.3.31", "dicomSupportedCharacterSet", "LDAP OID"},
    { "1.2.840.10008.15.0.4.1", "dicomConfigurationRoot", "LDAP OID"},
    { "1.2.840.10008.15.0.4.2", "dicomDevicesRoot", "LDAP OID"},
    { "1.2.840.10008.15.0.4.3", "dicomUniqueAETitlesRegistryRoot", "LDAP OID"},
    { "1.2.840.10008.15.0.4.4", "dicomDevice", "LDAP OID"},
    { "1.2.840.10008.15.0.4.5", "dicomNetworkAE", "LDAP OID"},
    { "1.2.840.10008.15.0.4.6", "dicomNetworkConnection", "LDAP OID"},
    { "1.2.840.10008.15.0.4.7", "dicomUniqueAETitle", "LDAP OID"},
    { "1.2.840.10008.15.0.4.8", "dicomTransferCapability", "LDAP OID"},
};

#endif  /* PACKET_DCM_H */