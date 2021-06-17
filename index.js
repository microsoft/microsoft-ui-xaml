var xhttp = new XMLHttpRequest();
xhttp.onreadystatechange =  function() {
    if (this.readyState == 4 && this.status == 200) {
        var xmlResp = this.responseXML;
        console.log(this.responseXML);
        // var js_markup = xmlResp.getElementsByTagName("javascriptIncludes")[0];

        // var uhfScripts = document.getElementById('uhf-scripts');
        // uhfScripts.innerHTML += js_markup.textContent;

        var header_markup = xmlResp.getElementsByTagName("headerHtml")[0];
        var footer_markup = xmlResp.getElementsByTagName("footerHtml")[0];


        if (document.getElementById("UHF-header") != null){
            document.getElementById("UHF-header").innerHTML += header_markup.textContent;
        }
        else {
            document.getElementById("UHF-header-about").innerHTML += header_markup.textContent;
        }

        if (document.getElementById("UHF-footer") != null){
            document.getElementById("UHF-footer").innerHTML += footer_markup.textContent;
        }
        else{
            document.getElementById("UHF-footer-about").innerHTML += footer_markup.textContent;
        }

        var css_markup = xmlResp.getElementsByTagName("cssIncludes")[0];

        var head = document.getElementsByTagName("head")[0];
        head.innerHTML += css_markup.textContent;
        console.log(header_markup.textContent);
    }
};
   
xhttp.open("GET", "https://uhf.microsoft.com/en-US/shell/xml/MSWinUI?headerId=MSWinUIHeader&footerid=MSWinUIFooter&CookieComplianceEnabled=true", true);
xhttp.send();

