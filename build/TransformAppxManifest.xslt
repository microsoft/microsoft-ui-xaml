<?xml version="1.0"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:ms="http://schemas.microsoft.com/appx/manifest/foundation/windows10">

<xsl:output method="xml" version="1.0" encoding="UTF-8" indent="yes"/>

<xsl:param name="ApplicationName" />

<xsl:template match="/">
  <assembly xmlns="urn:schemas-microsoft-com:asm.v1" manifestVersion="1.0">
    <xsl:for-each select="ms:Package/ms:Extensions/ms:Extension">
      <file>
        <xsl:attribute name="name">
          <xsl:value-of select="ms:InProcessServer/ms:Path" />
        </xsl:attribute>
        <xsl:for-each select="ms:InProcessServer/ms:ActivatableClass">
          <activatableClass xmlns="urn:schemas-microsoft-com:winrt.v1">
            <xsl:attribute name="name">
              <xsl:value-of select="@ActivatableClassId" />
            </xsl:attribute>
            <xsl:attribute name="threadingModel">
              <xsl:value-of select="@ThreadingModel" />
            </xsl:attribute>
          </activatableClass>
        </xsl:for-each>
      </file>
    </xsl:for-each>
  </assembly>
</xsl:template>

</xsl:stylesheet>
