/// <reference path="../../web1/lib1/innovaphone.lib1.js" />
/// <reference path="../../web1/ui1.lib/innovaphone.ui1.lib.js" />
/// <reference path="../../web1/ui1.svg/innovaphone.ui1.svg.js" />
/// <reference path="../../web1/ui1.autocompleteinput/innovaphone.ui1.autocompleteinput.js" />

innovaphone.TranscriptionBox = innovaphone.TranscriptionBox || function (main, language, confidenceFilter) {
    var that = this;
    var texts = [];

    that.createNode("div", null, null, "TranscriptionBox");

    that.OutputNode = that.add(new innovaphone.ui1.Node("div", "margin: 1%;", null, null));

    that.SettingsDiv = that.OutputNode.add(new innovaphone.ui1.Div("height: 30px; padding: 5px;", null, "settingsDiv"));
    that.languageSwitch = that.SettingsDiv.add(new innovaphone.ui1.Switch("margin: 5px; float: left;", "20px", "30px", true));
    var languageInputConfig = new innovaphone.ui1.AutoCompleteInputConfig(null, "small", null, "suggest", "suggest-highlight", "suggest-dropdown");
    that.languageInput = that.SettingsDiv.add(new innovaphone.ui1.AutoCompleteInput("float: left; margin: 5px", null, languageInputConfig));
    this.languageInput.setValue(language);

    that.OutputDiv = that.OutputNode.add(new innovaphone.ui1.Div("min-height: 300px; border-color: white; border: solid; padding: 5px;", null, "OutputDiv"));

    that.highlight = function (text_index, highlight) {
        texts[text_index].highlight(highlight);
    }

    that.checkTimestamp = function (timestamp) {
        if (texts[texts.length - 1] && texts[texts.length - 1].ts > timestamp - 300 && texts[texts.length - 1].ts < timestamp + 300) {
            return texts.length - 1;
        } else {
            return false;
        }
    }

    that.getHeight = function (textBoxId) {
        return texts[textBoxId].getHeight();
    }

    that.setHeight = function (textBoxId, height) {
        return texts[textBoxId].setHeight(height);
    }

    that.checkConfidence = function (textBoxId, confidence) {
        if (texts[textBoxId].confidence != false && texts[textBoxId].confidence >= confidence) {
            return true;
        } else {
            return false;
        }
    }

    that.getConfidence = function (textBoxId) {
        if (texts[textBoxId].confidence != false) {
            return texts[textBoxId].confidence;
        } else {
            return 0;
        }
    }

    that.addText = function (timestamp, confidence, text) {
        var correct = false;

        if (texts[texts.length - 1] && texts[texts.length - 1].ts > timestamp - 300 && texts[texts.length - 1].ts < timestamp + 300) {
            texts[texts.length - 1].updateText(timestamp, correct, confidence, text);
        } else {
            var textbox = new innovaphone.textbox(timestamp, correct, confidence, text);
            texts.push(textbox);
            that.OutputDiv.add(textbox);
        }
    }

    that.addTranslation = function (timestamp, language, translation) {

        texts.forEach(function (element, index) {
            if (element.ts == timestamp) {
                element.addTranslation(language, translation);
            }
        });
    }

    onSwitchCLick = function (e) {
        //var state = e.currentTarget.state;
    }
    that.languageSwitch.addEvent("click", onSwitchCLick);

    function languages(value) {
        var languages = ["ar-MS", "de-DE", "en-AU", "en-GB", "en-US", "es-ES", "fr-CA", "fr-FR", "it-IT", "nl-BE", "pt-BR"];
        var resultArray = [];
        for (var i = 0; i < languages.length; i++) {
            if (languages[i].toLowerCase().startsWith(value.toLowerCase())) {
                resultArray.push(languages[i]);
            }
        }
        return resultArray;
    }

    that.languageInput.setDataCallBack(languages);


}
innovaphone.TranscriptionBox.prototype = innovaphone.ui1.nodePrototype;

innovaphone.textbox = innovaphone.textbox || function (timestamp, correct, confidence, text) {
    var that = this;
    that.ts = timestamp;
    that.confidence = confidence;
    that.higlighted = false;
    that.text = text;
    that.translation = "";

    that.createNode("div", null, null, "textbox");
    
    if (correct) {
        that.setStyle("background-color", "green");
        that.higlighted = true;
    }

    if (text != "") {
        that.addHTML("<span>" + confidence + ": " + text + "</span>");
    }

    that.getHeight = function () {
        return that.container.clientHeight;
    }

    that.setHeight = function (height) {
        that.setStyle("min-height", height.toString() + "px");
    }

    that.highlight = function (higlight) {
        if (higlight) {
            that.setStyle("background-color", "green");
            that.higlighted = true;
        } else {
            that.setStyle("background-color", "transparent");
            that.higlighted = false;
        }
    }

    that.updateText = function (timestamp, correct, confidence, text) {
        if (text != "") {
            that.ts = timestamp;
            that.confidence = confidence;
            that.text = text;

            that.addHTML("<span>" + confidence + ": " + text + "</span>");

            if (correct) {
                that.setStyle("background-color", "green");
            }
        }
    }

    that.addTranslation = function (language, translation) {
        console.warn("language " + language + " translation " + translation);
        that.translation = that.translation + "<br><i>" + language + ": " + translation + "</i>";
        that.addHTML("<span>" + that.confidence + ": " + that.text + that.translation + "</span>");
    }
}
innovaphone.textbox.prototype = innovaphone.ui1.nodePrototype;