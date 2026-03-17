<?xml version="1.0" encoding="utf-8"?>
<xsl:stylesheet version="1.0"
  xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
  xmlns:wix="http://schemas.microsoft.com/wix/2006/wi">
  <xsl:output method="xml" indent="yes" omit-xml-declaration="no" />
  <xsl:strip-space elements="*" />

  <!-- Components already listed in TeenAstroEmulator.wxs (MainUnit, SHC, SDL2, Uploader, icon). -->
  <xsl:key name="excluded" match="wix:Component[
    contains(wix:File/@Source, 'TeenAstroMainUnit.exe') or
    contains(wix:File/@Source, 'TeenAstroSHC.exe') or
    contains(wix:File/@Source, 'SDL2.dll') or
    contains(wix:File/@Source, 'TeenAstroUploader.exe') or
    contains(wix:File/@Source, 'icon.ico')
  ]" use="@Id" />

  <xsl:template match="wix:Component[key('excluded', @Id)]" />
  <xsl:template match="wix:ComponentRef[key('excluded', @Id)]" />

  <xsl:template match="@*|node()">
    <xsl:copy>
      <xsl:apply-templates select="@*|node()" />
    </xsl:copy>
  </xsl:template>
</xsl:stylesheet>
