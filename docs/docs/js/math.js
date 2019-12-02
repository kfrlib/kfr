(function () {
    'use strict';

    var katexMath = (function () {
        var maths = document.querySelectorAll('.arithmatex'),
            tex;

        for (var i = 0; i < maths.length; i++) {
            tex = maths[i].textContent || maths[i].innerText;
            if (tex.startsWith('\\(') && tex.endsWith('\\)')) {
                katex.render(tex.slice(2, -2), maths[i], { 'displayMode': false });
            } else if (tex.startsWith('\\[') && tex.endsWith('\\]')) {
                katex.render(tex.slice(2, -2), maths[i], { 'displayMode': true });
            }
        }
    });

    (function () {
        var onReady = function onReady(fn) {
            if (document.addEventListener) {
                document.addEventListener("DOMContentLoaded", fn);
            } else {
                document.attachEvent("onreadystatechange", function () {
                    if (document.readyState === "interactive") {
                        fn();
                    }
                });
            }
        };

        onReady(function () {
            if (typeof katex !== "undefined") {
                katexMath();
            }
        });
    })();
    
    /*function load_navpane() {
        var width = window.innerWidth;
        if (width <= 1200) {
            return;
        }
    
        var nav = document.getElementsByClassName("md-nav");
        for(var i = 0; i < nav.length; i++) {
            if (typeof nav.item(i).style === "undefined") {
                continue;
            }
    
            if (nav.item(i).getAttribute("data-md-level") && nav.item(i).getAttribute("data-md-component")) {
                nav.item(i).style.display = 'block';
                nav.item(i).style.overflow = 'visible';
            }
        }
    
        var nav = document.getElementsByClassName("md-nav__toggle");
        for(var i = 0; i < nav.length; i++) {
           nav.item(i).checked = true;
        }
    }

    document.addEventListener("DOMContentLoaded", function() {
        load_navpane();
    });*/

}());
