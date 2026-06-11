<?xml version="1.0"?>
<!-- Copyright (c) Microsoft Corporation. Licensed under the MIT License. See LICENSE in the project root for license information. -->

<xsl:stylesheet version="1.0" xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10" xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
    xmlns:asmv1="urn:schemas-microsoft-com:asm.v1" xmlns:asmv3="urn:schemas-microsoft-com:asm.v3" xmlns:winrtv1="urn:schemas-microsoft-com:winrt.v1" exclude-result-prefixes="asmv1 asmv3 winrtv1">

    <xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"/>

    <xsl:template match="/">
        <Extensions>
            <!-- for now, skip comInterfaceProxyStub elements and only transform activatableClass elements -->
            <xsl:for-each select="asmv1:assembly/asmv3:file[winrtv1:activatableClass]">
                <Extension Category="windows.activatableClass.inProcessServer">
                    <InProcessServer>
                        <Path>
                            <xsl:value-of select="@name" />
                        </Path>
                        <xsl:for-each select="winrtv1:activatableClass">
                            <ActivatableClass>
                                <xsl:attribute name="ActivatableClassId">
                                    <xsl:value-of select="@name" />
                                </xsl:attribute>
                                <xsl:attribute name="ThreadingModel">
                                    <xsl:value-of select="@threadingModel" />
                                </xsl:attribute>
                            </ActivatableClass>
                        </xsl:for-each>
                    </InProcessServer>
                </Extension>
            </xsl:for-each>
        </Extensions>
    </xsl:template>

</xsl:stylesheet>
