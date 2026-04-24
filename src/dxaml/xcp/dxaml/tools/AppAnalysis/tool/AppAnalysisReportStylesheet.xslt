<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
  <!--<xsl:output method="html"/>-->
  
<xsl:key name="notification-by-ruleid" match="Notification" use="@RuleId"/> 
  <xsl:template match="AppAnalysisReport">
    <html>
      <head>
        <title>App Analysis Report</title>
        <style type="text/css">
          body {font-family: Segoe UI;}
          td.failed { color: red; font-weight: bold; }
          span.passed { color: green; font-weight: bold; }
          span.failed { color: red; font-weight: bold; }
          span.strong { font-weight: bolder; }
          td.header { background:#efeeef; color:gray; font-weight: bold;}
          td.ruleheader { padding-left:7px; font-weight: bold; }
          td.instanceDescription { padding-left:20px; }
          h1.title {  }
          tr.rule { background: #0078D7; color:white}
          tr.description { background: #ebf8fe}
          tr.instance { background:#f8f8f8; }
          tr.separator {height:15px}
          table {border-collapse:collapse}
        </style>
      </head>
      <body>
        <h1 class="title">App Analysis Report</h1>
        <p>
          Report Generated: <xsl:apply-templates select="/AppAnalysisReport/@DateTime"/>
        </p>
        <p>
          Overall Result:
          <xsl:variable name="NumFailed" select="count(//Notification)"/>
          <xsl:choose>
            <xsl:when test="$NumFailed > 0">
              <span class="failed">FAILED</span>
            </xsl:when>
            <xsl:otherwise>
              <span class="passed">PASSED</span>
            </xsl:otherwise>
          </xsl:choose>
        </p>
        <br/>
        
        <!-- List rules -->
        <table>
          <tr>
            <td colspan="2">
              <h3>
                <u>Rules Executed</u>
              </h3>
            </td>
          </tr>
          <tr>
            <td class="header">Id</td>
            <td class="header">Title</td>
          </tr>
          <xsl:for-each select="//Rule">
            <tr class="instance">
              <td>
                <xsl:value-of select="./@Id"/>
              </td>
              <td class="instanceDescription">
                <xsl:value-of select="./@Title"/>
              </td>
            </tr>
          </xsl:for-each>
        </table>
        <br/>
        
        <!-- List notifications -->
        
        <table>
          <tr>
            <td colspan="5">
              <h3>
                <u>Notifications</u>
              </h3>
            </td>
          </tr>
          
          <!-- Muenchian for-each-group -->
            <xsl:for-each select="//Notification[count(. | key('notification-by-ruleid', @RuleId)[1]) = 1]">
                <xsl:variable name="current-grouping-key" 
                            select="@RuleId"/>
                <xsl:variable name="current-group" 
                            select="key('notification-by-ruleid', 
                                        $current-grouping-key)"/>
                <tr class="separator"/><td colspan="5"/><tr/>
                                         
                <!-- Group headers -->
                <tr class="rule">
                    <td class="ruleheader"><xsl:value-of select="@RuleId" /></td>
                    <td class="ruleheader" colspan="4"><xsl:value-of select="@RuleTitle" /></td>
                </tr>
                <tr class="description">
                    <td colspan="2"/>
                    <td colspan="3"><span class="strong">Description:</span> &#160;<xsl:value-of select="Description"/></td>            
                </tr>
                <tr class="description">
                    <td colspan="2"/>
                    <td colspan="3"><span class="strong">Impact:</span> &#160;<xsl:value-of select="Impact"/></td>             
                </tr>
                <tr class="description">
                    <td colspan="2"/>
                    <td colspan="3"><span class="strong">Solution:</span> &#160;<xsl:value-of select="Solution"/></td>             
                </tr>
                <tr class="description">
                    <td colspan="2"/>
                    <td colspan="3"><span class="strong">MSDN reference:</span> &#160;<a href="{Information/References/Link/@URL}"><xsl:value-of select="Information/References/Link/@URL"/></a></td>             
                </tr>
 
                <tr>
                    <td class="header"></td>
                    <td class="header" colspan="1">Measurement</td>
                    <td class="header" colspan="2">Instance Cause</td>
                    <td class="header">Source Info</td>
                </tr>
                                  
                <!-- Group content -->
                <xsl:for-each select="$current-group">
  
                    <tr class="instance">
                        <td/>
                        <td><xsl:value-of select="Information/Measurement/@Value"/>&#160;<xsl:value-of select="Information/Measurement/@Unit"/></td>
                        
                        <!-- Conditionally retrieve info for different xml structures when we have different children... -->
                        <xsl:variable name="NumFiles" select="count(Information/File)"/>
                        <xsl:variable name="NumChildren" select="count(Information/Children/Child)"/>
                        <xsl:choose>
                            <xsl:when test="$NumFiles > 0">
                                <td colspan="2" class="instanceDescription"><xsl:value-of select="Information/@Description"/></td>
                                <td><xsl:value-of select="Information/File/@Name"/> (Line: <xsl:value-of select="Information/File/@Line"/>, Col: <xsl:value-of select="Information/File/@Column"/>)</td>
                            </xsl:when>
                            <xsl:when test="$NumChildren > 0">
                                <td colspan="2" class="instanceDescription">
                                    <xsl:for-each select="./Information/Children/Child[@Title='Parent Element']">
                                        Parent Element: &#160;<xsl:value-of select="@Description"/><br/>
                                    </xsl:for-each>
                                    <xsl:choose>
                                        <xsl:when test="count(Information/Children/Child[@Title='Reason']) > 0">
                                        Cause: &#160;<xsl:value-of select="./Information/Children/Child[@Title='Reason']/@Description"/><br/>
                                        </xsl:when>
                                    </xsl:choose>
                                </td>
                                <td>
                                    <xsl:for-each select="./Information/Children/Child/File">
                                        <xsl:value-of select="@Name"/> (Line: <xsl:value-of select="@Line"/>, Col: <xsl:value-of select="@Column"/>)
                                        <br/>
                                    </xsl:for-each>
                                </td>
                            </xsl:when>
                            <xsl:otherwise>
                                <td colspan="2" class="instanceDescription"><xsl:value-of select="Information/@Description"/>
                                </td><td/>    
                            </xsl:otherwise>
                        </xsl:choose>
                    </tr>
                </xsl:for-each>
            </xsl:for-each>
        </table>
      </body>
    </html>
  </xsl:template>
</xsl:stylesheet>
