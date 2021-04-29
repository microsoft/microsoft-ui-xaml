
var xhttp = new XMLHttpRequest();
xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
        var xmlResp = this.responseXML;
        var header_markup = xmlResp.getElementsByTagName("headerHtml")[0];
        document.getElementById("UHF-header").innerHTML = header_markup.textContent;

        var footer_markup = xmlResp.getElementsByTagName("footerHtml")[0];
        document.getElementById("UHF-footer").innerHTML = footer_markup.textContent;
    }
};
xhttp.open("GET", "https://uhf.microsoft.com/en-US/shell/xml/MSWinUI?headerId=MSWinUIHeader&footerid=MSWinUIFooter&CookieComplianceEnabled=true", true);
xhttp.send();

function Include(type, src) {
    var head = document.getElementsByTagName('head')[0];
    var link;
    if (type.indexOf('css') > -1) {
        link = document.createElement('link');
        link.rel = 'stylesheet';
        link.href = src;
    } else {
        link = document.createElement('script')
        link.src = src;
    }
    link.type = type;
    head.appendChild(link);
}

fetch('https://uhf.microsoft.com/en-us/shell/api/mscc?sitename=WinUI&domain=https://microsoft.github.io/microsoft-ui-xaml/&country=detect')
.then(response => response.json())
.then(function (data) {
    document.body.insertAdjacentHTML('beforebegin', data.Markup)
    Include('text/css', data.Css);
    Include('text/javascript', data.Js);
});

