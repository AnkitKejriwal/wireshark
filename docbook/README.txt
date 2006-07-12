$Id$

This directory contains the source files needed to build the:

Wireshark User's guide

and the:

Wireshark Developer's Guide  (in an early state).

To build both Guide's, just do 'make', but see requirements below.


The guides are written in Docbook/XML (formerly Docbook/SGML). This format is 
now used by many other documentation projects, e.g. "the linux documentation 
project" uses it too.

To get HTML, PDF or other output formats, conversions are done using XSL 
stylesheets, which provides a flexible way for these conversions.

The current Makefile is running under Win32 in the cygwin environment, so it uses 
GNU make and such. It should be pretty easy to use it in UNIX environments too.
Using Microsoft make (nmake) is not supported.

By default the Makefile generates HTML in single page and multiple (chunked) formats 
and PDF. The optional output format CHM has to be enabled in the Makefile.


Requirements:
-------------

Settings in Makefile and catalog.xml
------------------------------------
You have to edit the settings in these files, to point to the DTD/XSL files, fop (and possibly hhc).

DocBook XML DTD
---------------
DocBook "official" XML DTD V4.2 from:
http://www.oasis-open.org/docbook/xml/
(or using cygwin package docbook-xml42)

DocBook XSL
-----------
The "official" XSL stylesheets from Norman Walsh:
http://docbook.sourceforge.net/
(or using cygwin package docbook-xsl)

xsltproc
--------
The XSL processor xsltproc. 
(it seems to be packages libxml2 and libxslt, ... please give comments)

FOP processor (for PDF generation only)
---------------------------------------
FOP processor from the apache project:
http://xml.apache.org/fop/
FOP is a JAVA program, so you need to have a JAVA environment installed.
I have put the fop-0.20.5 dir right into the sources dir. If you have it somewhere else,
you'll have to change the setting in the Makefile.

Be sure to also have installed JAI and/or jimi to be able to use/convert the png graphics files.
The fop release note webpage tells how to do it: 
download jimi from:
http://java.sun.com/products/jimi/
then extract the archive, then copy JimiProClasses.zip to FOP's lib dir and rename it to jimi-1.0.jar.

As I got OutOfMemoryException when running fop, I had to insert -Xmx256m into the last line of the fop.bat file from:
java -cp "%LOCALCLASSPATH%" org.apache.fop.apps.Fop %1 %2 %3 %4 %5 %6 %7 %8
to:
java -Xmx256m -cp "%LOCALCLASSPATH%" org.apache.fop.apps.Fop %1 %2 %3 %4 %5 %6 %7 %8
This should be added automatically on unixish systems.

HTML help compiler (for chm file generation only)
-------------------------------------------------
hhc compiler from Microsoft:
http://msdn.microsoft.com/library/default.asp?url=/library/en-us/htmlhelp/html/hwMicrosoftHTMLHelpDownloads.asp 

Packages for Suse 9.3
---------------------
Tool/File	Package
---------	-------
xsltproc:	libxslt
xmllint:	libxml2
fop:		fop
docbook.xsl:	docbook-xsl-stylesheets
chunk.xsl:	docbook-xsl-stylesheets
htmlhelp.xsl:	docbook-xsl-stylesheets
docbookx.dtd:	docbook_4
jimi:		N/A - build yourself - see above

Packages for Gentoo
-------------------
Like with all packages do...
Check dependencies: emerge -p <package>
Install it:         emerge <package>

Tool/File	Package                  Opt./Mand.   Comments
---------	-------                  ----------   --------
xsltproc:	libxslt                  M            XSLT processer. A very fast processor writtin in C.
xmllint:	libxml2                  O            Xml Validator. You probably want to install that.
fop:		fop                      O            Only needed to generate PDFs. Has a lot of JAVA dependencies.
docbook.xsl:	docbook-xsl-stylesheets  M            Necessary docbook catalogs are built automatically by portage in /etc/xml and /etc/sgml
chunk.xsl:	docbook-xsl-stylesheets  M              using "/usr/bin/build-docbook-catalog".
htmlhelp.xsl:	docbook-xsl-stylesheets  M              So docbook runs out of the box on Gentoo.
docbookx.dtd:	docbook-xml-dtd          M
jimi:		sun-jimi                 O            Jimi is a class library for managing images. Used by fop.
Quanta+         quanta or kdewebdev      O            Nice HTML/XML/SGML and Docbook editor with Syntaxhighlighting, Autocompletion, etc.

Tip: The actual DTD version of Gentoo is 4.4, but wireshark docs still use 4.2.
     To be able to generate the docs, change the version in the second line of developer-guide.xml
     or install an older version of the DTD.
     See into the Gentoo handbook howto unmask old versions.

Packages for Fedora Core
------------------------
TODO

Packages for Debian
-------------------
Tool/File	Package
---------	-------
xsltproc:	libxslt
xmllint:	libxml2-utils
fop:		fop
docbook.xsl:	docbook-xsl
chunk.xsl:	docbook-xsl
htmlhelp.xsl:	docbook-xsl
docbookx.dtd:	docbook-xml
jimi:		N/A - build yourself - see above

Makefile:
---------
There are several ways and tools to do these conversion, following is a short 
description of the way the makefile targets are doing things and which output 
files required for a release in that format.

all
Will generate both guide's in all available output formats (see below).

make wsug
Will generate Wireshark User's Guide in all available output formats.

make wsug_html
The HTML file is generated using xsltproc and the XSL stylesheets from 
Norman Walsh. This is a conversion into a single HTML page.
output: wsug_html

make wsug_html_chunked
The HTML files are generated using xsltproc and the XSL stylesheets from 
Norman Walsh. This is a conversion into chunked (multiple) HTML pages.
output: wsug_html_chunked

make wsug_pdf_us
make wsug_pdf_a4
The PDF is generated using an intermediate format named XSL-FO (XSL 
formatting objects). xsltproc converts the XML to a FO file, and then fop 
(apache's formatting object processor) is used to generate the PDF document, 
in US letter or A4 paper format.
TIP: You will get lot's of INFO/WARNING/ERROR messages when generating pdf, 
but conversation works just fine.
output: user-guide-us.pdf user-guide-a4.pdf

make wsug_chm
On Win32 platforms, the "famous" HTML help format can be generated by using a 
special HTML chunked conversion and then use the htmlhelp compiler from 
Microsoft.
output: htmlhelp.chm

Using the prefix wsdg_ instead of wsug_ will build the same targets but for the 
Wireshark Developer's Guide.

The makefile is written to be run with gmake on UNIX/Linux platforms. Win32 
platforms have to use the cygwin environment (Microsoft nmake is not 
supported).


Docbook web references:
-----------------------
Some web references to further documentation about Docbook/XML and Docbook XSL conversions:

DocBook: The Definitive Guide
by Norman Walsh and Leonard Muellner
http://www.docbook.org/tdg/en/html/docbook.html

DocBook XSL: The Complete Guide
by Bob Stayton
http://www.sagehill.net/docbookxsl/index.html

Documention with DocBook on Win32
by Jim Crafton
http://www.codeproject.com/winhelp/docbook_howto.asp

FO Parameter Reference
by Norman Walsh
http://docbook.sourceforge.net/release/xsl/current/doc/fo/

