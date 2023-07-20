
/// <reference path="../../web1/lib1/innovaphone.lib1.js" />
/// <reference path="../../web1/ui1.lib/innovaphone.ui1.lib.js" />
/// <reference path="../../web1/ui1.svg/innovaphone.ui1.svg.js" />

innovaphone.SearchBar = innovaphone.SearchBar || function (main) {
    var that = this;
    //var texts = main.texts;
    var svgLib = main.svgLib;
    var callActive = false;

    that.createNode("div", null, null, "SearchBar");

    that.add(new innovaphone.ui1.Node("div", "white-space:nowrap;", null, null));
    that.InputDiv = that.add(new innovaphone.ui1.Div(null, null, "SearchBarInputDiv"));

    that.Input = that.InputDiv.add(new innovaphone.ui1.Node("input", null, null, "SearchBarInput"));
    //that.InputButton = that.InputDiv.add(new innovaphone.ui1.SvgInline("width:22px;height:22px;padding:10px;fill:var(--main-c2);cursor:pointer", "0 0 20 20", svgLib.get("Search"), "InputButton"));
    //that.InputButton.addEvent("click", function (ev, obj) { that.ClearInputControl(); that.Input.container.focus() });

    if (!callActive) {
        that.CallButton = that.InputDiv.add(new innovaphone.ui1.SvgInline("width:22px;height:22px;padding:10px;fill:var(--main-c2);cursor:pointer", "0 0 20 20", svgLib.get("Call"), "CallButton"));
    }
    else {
        that.CallButton = that.InputDiv.add(new innovaphone.ui1.SvgInline("width:22px;height:22px;padding:10px;fill:var(--main-c2);cursor:pointer;display:none", "0 0 20 20", svgLib.get("HangUp"), "HangUpButton"));
    }
    that.CallButton.addEvent("click", function (ev, obj) { StartCallButtonClicked(ev, obj) });

    this.CallButonIcon = function(icon) {
        console.log("CallButtonIcon() icon: " + icon);
        if (icon == "HangUp") {
            callActive = true;
        }
        else if (icon == "Call") {
            callActive = false;

            that.Input.container.value = "";
        }
        that.CallButton.setCode(svgLib.get(icon));
    }


    function StartCallButtonClicked(ev, obj) {
        if (!callActive) {
            var input = that.Input.container.value;

            main.Dial(input);
        }
        else {
            //that.CallButonIcon("Call");

            main.HangUp();
        }

    }
}
innovaphone.SearchBar.prototype = innovaphone.ui1.nodePrototype;