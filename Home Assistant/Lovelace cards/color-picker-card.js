function e(e){return e&&e.__esModule?e.default:e}var t,r,l={};t="undefined"!=typeof self?self:void 0,r=function(){/*!
  * Copyright (c) 2021-2023 Momo Bassit.
  * Licensed under the MIT License (MIT)
  * https://github.com/mdbassit/Coloris
  * Version: 0.21.1
  * NPM: https://github.com/melloware/coloris-npm
  */return((e,t,r,l)=>{let a=t.createElement("canvas").getContext("2d"),o={r:0,g:0,b:0,h:0,s:0,v:0,a:1},i,n,c,s,d,p,h,u,b,f,g,m,x,v,w,y,k={},E={el:"[data-coloris]",parent:"body",theme:"default",themeMode:"light",rtl:!1,wrap:!0,margin:2,format:"hex",formatToggle:!1,swatches:[],swatchesOnly:!1,alpha:!0,forceAlpha:!1,focusInput:!0,selectInput:!1,inline:!1,defaultColor:"#000000",clearButton:!1,clearLabel:"Clear",closeButton:!1,closeLabel:"Close",onChange:()=>l,a11y:{open:"Open color picker",close:"Close color picker",clear:"Clear the selected color",marker:"Saturation: {s}. Brightness: {v}.",hueSlider:"Hue slider",alphaSlider:"Opacity slider",input:"Color value field",format:"Color format",swatch:"Color swatch",instruction:"Saturation and brightness selector. Use up, down, left and right arrow keys to select."}},L={},C="",$={},_=!1;function S(t,r){if("object"==typeof t)for(let a in t)switch(a){case"el":H(t.el),!1!==t.wrap&&j(t.el);break;case"parent":(i=r.querySelector(t.parent))&&(i.appendChild(n),E.parent=t.parent,i===r.body&&(i=l));break;case"themeMode":E.themeMode=t.themeMode,"auto"===t.themeMode&&e.matchMedia&&e.matchMedia("(prefers-color-scheme: dark)").matches&&(E.themeMode="dark");case"theme":t.theme&&(E.theme=t.theme),n.className=`clr-picker clr-${E.theme} clr-${E.themeMode}`,E.inline&&D(r);break;case"rtl":E.rtl=!!t.rtl,r.querySelectorAll(".clr-field").forEach(e=>e.classList.toggle("clr-rtl",E.rtl));break;case"margin":t.margin*=1,E.margin=isNaN(t.margin)?E.margin:t.margin;break;case"wrap":t.el&&t.wrap&&j(t.el);break;case"formatToggle":E.formatToggle=!!t.formatToggle,J("clr-format",r).style.display=E.formatToggle?"block":"none",E.formatToggle&&(E.format="auto");break;case"swatches":if(Array.isArray(t.swatches)){let e=[];t.swatches.forEach((t,r)=>{e.push(`<button type="button" id="clr-swatch-${r}" aria-labelledby="clr-swatch-label clr-swatch-${r}" style="color: ${t};">${t}</button>`)}),J("clr-swatches",r).innerHTML=e.length?`<div>${e.join("")}</div>`:"",E.swatches=t.swatches.slice()}break;case"swatchesOnly":E.swatchesOnly=!!t.swatchesOnly,n.setAttribute("data-minimal",E.swatchesOnly);break;case"alpha":E.alpha=!!t.alpha,n.setAttribute("data-alpha",E.alpha);break;case"inline":if(E.inline=!!t.inline,n.setAttribute("data-inline",E.inline),E.inline){let e=t.defaultColor||E.defaultColor;v=R(e),D(r),O(e,r)}break;case"clearButton":"object"==typeof t.clearButton&&(t.clearButton.label&&(E.clearLabel=t.clearButton.label,h.innerHTML=E.clearLabel),t.clearButton=t.clearButton.show),E.clearButton=!!t.clearButton,h.style.display=E.clearButton?"block":"none";break;case"clearLabel":E.clearLabel=t.clearLabel,h.innerHTML=E.clearLabel;break;case"closeButton":E.closeButton=!!t.closeButton,E.closeButton?n.insertBefore(u,d):d.appendChild(u);break;case"closeLabel":E.closeLabel=t.closeLabel,u.innerHTML=E.closeLabel;break;case"a11y":let o=t.a11y,s=!1;if("object"==typeof o)for(let e in o)o[e]&&E.a11y[e]&&(E.a11y[e]=o[e],s=!0);if(s){let e=J("clr-open-label",r),t=J("clr-swatch-label",r);e.innerHTML=E.a11y.open,t.innerHTML=E.a11y.swatch,u.setAttribute("aria-label",E.a11y.close),h.setAttribute("aria-label",E.a11y.clear),b.setAttribute("aria-label",E.a11y.hueSlider),g.setAttribute("aria-label",E.a11y.alphaSlider),p.setAttribute("aria-label",E.a11y.input),c.setAttribute("aria-label",E.a11y.instruction)}break;default:E[a]=t[a]}}function A(e,t){"string"==typeof e&&"object"==typeof t&&(L[e]=t,_=!0)}function T(e){delete L[e],0===Object.keys(L).length&&(_=!1,e===C&&M())}function B(e){if(_){let t=["el","wrap","rtl","inline","defaultColor","a11y"];for(let r in L){let l=L[r];if(e.matches(r)){for(let e in C=r,$={},t.forEach(e=>delete l[e]),l)$[e]=Array.isArray(E[e])?E[e].slice():E[e];S(l);break}}}}function M(){Object.keys($).length>0&&(S($),C="",$={})}function H(e,r){K(t,"click",e,e=>{E.inline||(B(e.target),v=R(w=(x=e.target).value),n.classList.add("clr-open"),D(r),O(w,r),(E.focusInput||E.selectInput)&&(p.focus({preventScroll:!0}),p.setSelectionRange(x.selectionStart,x.selectionEnd)),E.selectInput&&p.select(),(y||E.swatchesOnly)&&U().shift().focus(),x.dispatchEvent(new Event("open",{bubbles:!0})))}),K(t,"input",e,e=>{let t=e.target.parentNode;t.classList.contains("clr-field")&&(t.style.color=e.target.value)})}function D(r){let l,a,o;if(!n||!x&&!E.inline)return;let s=i,d=e.scrollY,p=n.offsetWidth,h=n.offsetHeight,u={left:!1,top:!1},b={x:0,y:0};if(s&&(a=parseFloat((l=e.getComputedStyle(s)).marginTop),o=parseFloat(l.borderTopWidth),b=s.getBoundingClientRect(),b.y+=o+d),!E.inline){let e=x.getBoundingClientRect(),r=e.x,l=d+e.y+e.height+E.margin;s?(r-=b.x,l-=b.y,r+p>s.clientWidth&&(r+=e.width-p,u.left=!0),l+h>s.clientHeight-a&&h+E.margin<=e.top-(b.y-d)&&(l-=e.height+h+2*E.margin,u.top=!0),l+=s.scrollTop):(r+p>t.documentElement.clientWidth&&(r+=e.width-p,u.left=!0),l+h-d>t.documentElement.clientHeight&&h+E.margin<=e.top&&(l=d+e.y-h-E.margin,u.top=!0)),n.classList.toggle("clr-left",u.left),n.classList.toggle("clr-top",u.top),n.style.left=`${r}px`,n.style.top=`${l}px`,b.x+=n.offsetLeft,b.y+=n.offsetTop}k={width:c.offsetWidth,height:c.offsetHeight,x:c.offsetLeft+b.x,y:c.offsetTop+b.y}}function j(e){t.querySelectorAll(e).forEach(e=>{let r=e.parentNode;if(!r.classList.contains("clr-field")){let l=t.createElement("div"),a="clr-field";(E.rtl||e.classList.contains("clr-rtl"))&&(a+=" clr-rtl"),l.innerHTML='<button type="button" aria-labelledby="clr-open-label"></button>',r.insertBefore(l,e),l.setAttribute("class",a),l.style.color=e.value,l.appendChild(e)}})}function z(e){if(x&&!E.inline){let t=x;e&&(x=l,w!==t.value&&(t.value=w,t.dispatchEvent(new Event("input",{bubbles:!0})))),setTimeout(()=>{w!==t.value&&t.dispatchEvent(new Event("change",{bubbles:!0}))}),n.classList.remove("clr-open"),_&&M(),t.dispatchEvent(new Event("close",{bubbles:!0})),E.focusInput&&t.focus({preventScroll:!0}),x=l}}function O(e,t){let l,o;let i=(a.fillStyle="#000",a.fillStyle=e,(l=/^((rgba)|rgb)[\D]+([\d.]+)[\D]+([\d.]+)[\D]+([\d.]+)[\D]*?([\d.]+|$)/i.exec(a.fillStyle))?(o={r:1*l[3],g:1*l[4],b:1*l[5],a:1*l[6]}).a=+o.a.toFixed(2):o={r:(l=a.fillStyle.replace("#","").match(/.{2}/g).map(e=>parseInt(e,16)))[0],g:l[1],b:l[2],a:1},o),c=function(e){let t=e.r/255,l=e.g/255,a=e.b/255,o=r.max(t,l,a),i=o-r.min(t,l,a),n=0,c=0;return i&&(o===t&&(n=(l-a)/i),o===l&&(n=2+(a-t)/i),o===a&&(n=4+(t-l)/i),o&&(c=i/o)),{h:(n=r.floor(60*n))<0?n+360:n,s:r.round(100*c),v:r.round(100*o),a:e.a}}(i);Y(c.s,c.v),P(i,c,t),b.value=c.h,n.style.color=`hsl(${c.h}, 100%, 50%)`,f.style.left=`${c.h/360*100}%`,s.style.left=`${k.width*c.s/100}px`,s.style.top=`${k.height-k.height*c.v/100}px`,g.value=100*c.a,m.style.left=`${100*c.a}%`}function R(e){let t=e.substring(0,3).toLowerCase();return"rgb"===t||"hsl"===t?t:"hex"}function I(r){r=r!==l?r:p.value,x&&(x.value=r,x.dispatchEvent(new Event("input",{bubbles:!0}))),E.onChange&&E.onChange.call(e,r,x),t.dispatchEvent(new CustomEvent("coloris:pick",{detail:{color:r,currentEl:x}}))}function N(e,t,l){let a={h:1*b.value,s:e/k.width*100,v:100-t/k.height*100,a:g.value/100},o=function(e){let t=e.s/100,l=e.v/100,a=t*l,o=e.h/60,i=a*(1-r.abs(o%2-1)),n=l-a;a+=n,i+=n;let c=r.floor(o)%6,s=[a,i,n,n,i,a][c],d=[i,a,a,i,n,n][c],p=[n,n,i,a,a,i][c];return{r:r.round(255*s),g:r.round(255*d),b:r.round(255*p),a:e.a}}(a);Y(a.s,a.v),P(o,a,l),I()}function Y(e,t){let r=E.a11y.marker;e=1*e.toFixed(1),t=1*t.toFixed(1),r=(r=r.replace("{s}",e)).replace("{v}",t),s.setAttribute("aria-label",r)}function q(e){let t={pageX:e.changedTouches?e.changedTouches[0].pageX:e.pageX,pageY:e.changedTouches?e.changedTouches[0].pageY:e.pageY},r=t.pageX-k.x,l=t.pageY-k.y;i&&(l+=i.scrollTop),F(r,l,this),e.preventDefault(),e.stopPropagation()}function F(e,t,r){e=e<0?0:e>k.width?k.width:e,t=t<0?0:t>k.height?k.height:t,s.style.left=`${e}px`,s.style.top=`${t}px`,N(e,t,r),s.focus()}function P(e,t,l){var a;void 0===e&&(e={}),void 0===t&&(t={});let i=E.format;for(let t in e)o[t]=e[t];for(let e in t)o[e]=t[e];let n=function(e){let t=e.r.toString(16),r=e.g.toString(16),l=e.b.toString(16),a="";if(e.r<16&&(t="0"+t),e.g<16&&(r="0"+r),e.b<16&&(l="0"+l),E.alpha&&(e.a<1||E.forceAlpha)){let t=255*e.a|0;a=t.toString(16),t<16&&(a="0"+a)}return"#"+t+r+l+a}(o),h=n.substring(0,7);switch(s.style.color=h,m.parentNode.style.color=h,m.style.color=n,d.style.color=n,c.style.display="none",c.offsetHeight,c.style.display="",m.nextElementSibling.style.display="none",m.nextElementSibling.offsetHeight,m.nextElementSibling.style.display="","mixed"===i?i=1===o.a?"hex":"rgb":"auto"===i&&(i=v),i){case"hex":p.value=n;break;case"rgb":p.value=E.alpha&&(1!==o.a||E.forceAlpha)?`rgba(${o.r}, ${o.g}, ${o.b}, ${o.a})`:`rgb(${o.r}, ${o.g}, ${o.b})`;break;case"hsl":p.value=(a=function(e){let t;let l=e.v/100,a=l*(1-e.s/100/2);return a>0&&a<1&&(t=r.round((l-a)/r.min(a,1-a)*100)),{h:e.h,s:t||0,l:r.round(100*a),a:e.a}}(o),E.alpha&&(1!==a.a||E.forceAlpha)?`hsla(${a.h}, ${a.s}%, ${a.l}%, ${a.a})`:`hsl(${a.h}, ${a.s}%, ${a.l}%)`)}l.querySelector(`.clr-format [value="${i}"]`).checked=!0}function W(){let e=1*b.value,t=1*s.style.left.replace("px",""),r=1*s.style.top.replace("px","");n.style.color=`hsl(${e}, 100%, 50%)`,f.style.left=`${e/360*100}%`,N(t,r,this)}function X(){let e=g.value/100;m.style.left=`${100*e}%`,P({a:e},{},this),I()}function G(e){e.getElementById("clr-picker")||(i=l,(n=t.createElement("div")).setAttribute("id","clr-picker"),n.className="clr-picker",n.innerHTML=`<input id="clr-color-value" name="clr-color-value" class="clr-color" type="text" value="" spellcheck="false" aria-label="${E.a11y.input}"><div id="clr-color-area" class="clr-gradient" role="application" aria-label="${E.a11y.instruction}"><div id="clr-color-marker" class="clr-marker" tabindex="0"></div></div><div class="clr-hue"><input id="clr-hue-slider" name="clr-hue-slider" type="range" min="0" max="360" step="1" aria-label="${E.a11y.hueSlider}"><div id="clr-hue-marker"></div></div><div class="clr-alpha"><input id="clr-alpha-slider" name="clr-alpha-slider" type="range" min="0" max="100" step="1" aria-label="${E.a11y.alphaSlider}"><div id="clr-alpha-marker"></div><span></span></div><div id="clr-format" class="clr-format"><fieldset class="clr-segmented"><legend>${E.a11y.format}</legend><input id="clr-f1" type="radio" name="clr-format" value="hex"><label for="clr-f1">Hex</label><input id="clr-f2" type="radio" name="clr-format" value="rgb"><label for="clr-f2">RGB</label><input id="clr-f3" type="radio" name="clr-format" value="hsl"><label for="clr-f3">HSL</label><span></span></fieldset></div><div id="clr-swatches" class="clr-swatches"></div><button type="button" id="clr-clear" class="clr-clear" aria-label="${E.a11y.clear}">${E.clearLabel}</button><div id="clr-color-preview" class="clr-preview"><button type="button" id="clr-close" class="clr-close" aria-label="${E.a11y.close}">${E.closeLabel}</button></div><span id="clr-open-label" hidden>${E.a11y.open}</span><span id="clr-swatch-label" hidden>${E.a11y.swatch}</span>`,e.appendChild(n),c=J("clr-color-area",e),s=J("clr-color-marker",e),h=J("clr-clear",e),u=J("clr-close",e),d=J("clr-color-preview",e),p=J("clr-color-value",e),b=J("clr-hue-slider",e),f=J("clr-hue-marker",e),g=J("clr-alpha-slider",e),m=J("clr-alpha-marker",e),H(E.el),j(E.el),K(n,"mousedown",e=>{n.classList.remove("clr-keyboard-nav"),e.stopPropagation()}),K(c,"mousedown",t=>{c.addEventListener("mousemove",q.bind(e))}),K(c,"touchstart",t=>{c.addEventListener("touchmove",q.bind(e),{passive:!1})}),K(s,"mousedown",t=>{s.addEventListener("mousemove",q.bind(e))}),K(s,"touchstart",t=>{s.addEventListener("touchmove",q.bind(e),{passive:!1})}),K(p,"change",t=>{let r=p.value;(x||E.inline)&&I(""===r?r:O(r,e))}),K(h,"click",e=>{I(""),z()}),K(u,"click",e=>{I(),z()}),K(J("clr-format",e),"click",".clr-format input",t=>{v=t.target.value,P({},{},e),I()}),K(n,"click",".clr-swatches button",t=>{O(t.target.textContent,e),I(),E.swatchesOnly&&z()}),K(c,"mouseup",t=>{c.removeEventListener("mousemove",q.bind(e))}),K(c,"touchend",t=>{c.removeEventListener("touchmove",q.bind(e))}),K(s,"mouseup",t=>{s.removeEventListener("mousemove",q.bind(e))}),K(s,"touchend",t=>{s.removeEventListener("touchmove",q.bind(e))}),K(t,"mousedown",e=>{y=!1,n.classList.remove("clr-keyboard-nav"),z()}),K(t,"keydown",e=>{let t=e.key,r=e.target,l=e.shiftKey;if("Escape"===t?z(!0):["Tab","ArrowUp","ArrowDown","ArrowLeft","ArrowRight"].includes(t)&&(y=!0,n.classList.add("clr-keyboard-nav")),"Tab"===t&&r.matches(".clr-picker *")){let t=U(),a=t.shift(),o=t.pop();l&&r===a?(o.focus(),e.preventDefault()):l||r!==o||(a.focus(),e.preventDefault())}}),K(t,"click",".clr-field button",e=>{_&&M(),e.target.nextElementSibling.dispatchEvent(new Event("click",{bubbles:!0}))}),K(s,"keydown",t=>{let r={ArrowUp:[0,-1],ArrowDown:[0,1],ArrowLeft:[-1,0],ArrowRight:[1,0]};Object.keys(r).includes(t.key)&&(function(e,t,r){F(1*s.style.left.replace("px","")+e,1*s.style.top.replace("px","")+t,r)}(...r[t.key],e),t.preventDefault())}),c.addEventListener("click",q.bind(e)),b.addEventListener("input",W.bind(e)),g.addEventListener("input",X.bind(e)))}function U(){return Array.from(n.querySelectorAll("input, button")).filter(e=>!!e.offsetWidth)}function J(e,t){return t.getElementById(e)}function K(e,t,r,l){let a=Element.prototype.matches||Element.prototype.msMatchesSelector;"string"==typeof r?e.addEventListener(t,e=>{a.call(e.target,r)&&l.call(e.target,e)}):(l=r,e.addEventListener(t,l))}function V(e,t){e(...t=t!==l?t:[])}function Z(e,t,r){w=(x=t).value,B(t),v=R(e),D(r),O(e,r),I(),w!==e&&x.dispatchEvent(new Event("change",{bubbles:!0}))}NodeList!==l&&NodeList.prototype&&!NodeList.prototype.forEach&&(NodeList.prototype.forEach=Array.prototype.forEach);let Q=(()=>{let t={init:G,set:S,wrap:j,close:z,setInstance:A,setColor:Z,removeInstance:T,updatePosition:D,ready:V};function r(e,t){V(()=>{e&&("string"==typeof e?H(e):S(e,t))})}for(let e in t)r[e]=function(){for(var r=arguments.length,l=Array(r),a=0;a<r;a++)l[a]=arguments[a];V(t[e],l)};return V(()=>{e.addEventListener("resize",e=>{r.updatePosition()}),e.addEventListener("scroll",e=>{r.updatePosition()})}),r})();return Q.coloris=Q,Q})(window,document,Math)},"function"==typeof define&&define.amd?define([],r):l?l=r():(t.Coloris=r(),"object"==typeof window&&t.Coloris.init());const a=(e,t,r,l)=>{l=l||{},r=null==r?{}:r;let a=new Event(t,{bubbles:void 0===l.bubbles||l.bubbles,cancelable:!!l.cancelable,composed:void 0===l.composed||l.composed});return a.detail=r,e.dispatchEvent(a),a};class o extends HTMLElement{constructor(){super(),this.attachShadow({mode:"open"}),this._current_color,this._last_state}setConfig(t){if(!t.device)throw Error("No Device Configured");this._config=function e(t){if(!(t&&"object"==typeof t))return t;if("[object Date]"==Object.prototype.toString.call(t))return new Date(t.getTime());if(Array.isArray(t))return t.map(e);var r={};return Object.keys(t).forEach(function(l){r[l]=e(t[l])}),r}(t),this._device=this._config.device,this._updated=!1;var r=this.shadowRoot;r.lastChild&&r.removeChild(r.lastChild);var o=document.createElement("style");o.textContent=this._cssData(),r.appendChild(o),this.hacard=document.createElement("ha-card"),this.hacard.className="f-ha-card",this.hacard.innerHTML=this._htmlData(),r.appendChild(this.hacard),r.querySelector(".refresh").addEventListener("click",()=>{e(l).init(r),e(l)({theme:"default",themeMode:"dark",format:"rgb",parent:".container",inline:!0,onChange:(e,t)=>{this._current_color=e,console.log(this._current_color)}},this.shadowRoot),a(window,"haptic","success")}),e(l).init(r),e(l)({theme:"default",themeMode:"dark",format:"rgb",parent:".container",inline:!0,onChange:(e,t)=>{this._current_color=e,console.log(this._current_color)}},this.shadowRoot),setTimeout(()=>{e(l)({theme:"default",themeMode:"dark",format:"rgb",parent:".container",inline:!0,onChange:(e,t)=>{this._current_color=e}},this.shadowRoot)},1e3)}set hass(e){this._hass=e,this._state=e.states[this._device],!1===this._updated&&(this.shadowRoot.getElementById("button-fg").addEventListener("click",()=>{e.callService("input_text","set_value",{entity_id:this._device[0],value:this._current_color}),a(window,"haptic","success")}),this.shadowRoot.getElementById("button-bg").addEventListener("click",()=>{e.callService("input_text","set_value",{entity_id:this._device[1],value:this._current_color}),a(window,"haptic","success")}),this._updated=!0)}_htmlData(){return`
        <div class="container"></div>
        <div class="button-row">
          <button class="apply" id="button-fg" type="button">FG</button>
          <button class="apply" id="button-bg" type="button">BG</button>
          <button class="refresh" type="button">Refresh</button>
        </div>  
      `}_cssData(){return`
      ha-card {
        border-radius: var(--border-radius);
        box-shadow: var(--box-shadow);
        padding: 12px;
        backdrop-filter: var(--bg-filter);   
        background-color: var(--card-blur-color);    
      }

      .button-row {
        display: grid;
        flex-shrink: 1;
        grid-template-columns: 1fr 1fr 1fr;
        grid-template-rows: 28px;        
        column-gap: 8px;
      }
      
      .apply, .refresh{
        background: rgba(var(--color-theme),0.05);
        color: rgba(var(--color-theme),0.85);
        box-shadow: none;
        border-style: none;
        border-radius: 14px;
        text-overflow: ellipsis;
        text-wrap: nowrap;
        margin: 0px;
        overflow: hidden;
        font-weight: bold;
      }

      .clr-picker {
        z-index: 1000;
        direction: ltr;
        -webkit-user-select: none;
        user-select: none;
        background-color: #fff;
        border-radius: calc(var(--border-radius) - 4px);
        flex-wrap: wrap;
        justify-content: space-around;
        display: none;
        position: absolute;
        box-shadow: none;
      }
      
      .clr-picker.clr-open, .clr-picker[data-inline="true"] {
        display: flex;
      }
      
      .clr-picker[data-inline="true"] {
        position: relative;
      }
      
      .clr-gradient {
        cursor: pointer;
        background-image: linear-gradient(#0000, #000), linear-gradient(90deg, #fff, currentColor);
        border-radius: 3px 3px 0 0;
        width: 100%;
        height: 100px;
        margin-bottom: 15px;
        position: relative;
      }
      
      .clr-marker {
        cursor: pointer;
        background-color: currentColor;
        border: 1px solid #fff;
        border-radius: 50%;
        width: 12px;
        height: 12px;
        margin: -6px 0 0 -6px;
        position: absolute;
      }
      
      .clr-picker input[type="range"]::-webkit-slider-runnable-track {
        width: 100%;
        height: 16px;
      }
      
      .clr-picker input[type="range"]::-webkit-slider-thumb {
        -webkit-appearance: none;
        width: 16px;
        height: 16px;
      }
      
      .clr-picker input[type="range"]::-moz-range-track {
        border: 0;
        width: 100%;
        height: 16px;
      }
      
      .clr-picker input[type="range"]::-moz-range-thumb {
        border: 0;
        width: 16px;
        height: 16px;
      }
      
      .clr-hue {
        background-image: linear-gradient(to right, red 0%, #ff0 16.66%, #0f0 33.33%, #0ff 50%, #00f 66.66%, #f0f 83.33%, red 100%);
      }
      
      .clr-hue {
        border-radius: 4px;
        width: calc(100%);
        height: 8px;
        margin: 0px 6px 8px 6px;
        position: relative;
      }

      .clr-alpha {
        border-radius: 4px;
        width: calc(100%);
        height: 8px;
        margin: 8px 6px 12px 6px;
        position: relative;
      }      

      .clr-alpha span {
        border-radius: inherit;
        background-image: linear-gradient(90deg, #0000, currentColor);
        width: 100%;
        height: 100%;
        display: block;
      }
      
      .clr-hue input, .clr-alpha input {
        opacity: 0;
        cursor: pointer;
        appearance: none;
        background-color: #0000;
        width: calc(100% + 32px);
        height: 16px;
        margin: 0;
        position: absolute;
        top: -4px;
        left: -16px;
      }
      
      .clr-hue div, .clr-alpha div {
        pointer-events: none;
        background-color: currentColor;
        border: 2px solid #fff;
        border-radius: 50%;
        width: 16px;
        height: 16px;
        margin-left: -8px;
        position: absolute;
        top: 50%;
        left: 0;
        transform: translateY(-50%);
        box-shadow: 0 0 1px #888;
      }
      
      .clr-alpha div:before {
        content: "";
        background-color: currentColor;
        border-radius: 50%;
        width: 100%;
        height: 100%;
        position: absolute;
        top: 0;
        left: 0;
      }
      
      .clr-format {
        order: 1;
        width: calc(100% - 40px);
        margin: 0 20px 20px;
        display: none;
      }
      
      .clr-segmented {
        box-sizing: border-box;
        color: #999;
        border: 1px solid #ddd;
        border-radius: 15px;
        width: 100%;
        margin: 0;
        padding: 0;
        font-size: 12px;
        display: flex;
        position: relative;
      }
      
      .clr-segmented input, .clr-segmented legend {
        opacity: 0;
        pointer-events: none;
        border: 0;
        width: 100%;
        height: 100%;
        margin: 0;
        padding: 0;
        position: absolute;
        top: 0;
        left: 0;
      }
      
      .clr-segmented label {
        font-size: inherit;
        font-weight: normal;
        line-height: initial;
        text-align: center;
        cursor: pointer;
        flex-grow: 1;
        margin: 0;
        padding: 4px 0;
      }
      
      .clr-segmented label:first-of-type {
        border-radius: 10px 0 0 10px;
      }
      
      .clr-segmented label:last-of-type {
        border-radius: 0 10px 10px 0;
      }
      
      .clr-segmented input:checked + label {
        color: #fff;
        background-color: #666;
      }
      
      .clr-swatches {
        order: 2;
        width: calc(100% - 32px);
        margin: 0 16px;
      }
      
      .clr-swatches div {
        flex-wrap: wrap;
        justify-content: center;
        padding-bottom: 12px;
        display: flex;
      }
      
      .clr-swatches button {
        color: inherit;
        text-indent: -1000px;
        white-space: nowrap;
        cursor: pointer;
        border: 0;
        border-radius: 50%;
        width: 20px;
        height: 20px;
        margin: 0 4px 6px;
        padding: 0;
        position: relative;
        overflow: hidden;
      }
      
      .clr-swatches button:after {
        content: "";
        border-radius: inherit;
        background-color: currentColor;
        width: 100%;
        height: 100%;
        display: block;
        position: absolute;
        top: 0;
        left: 0;
        box-shadow: inset 0 0 0 1px #0000001a;
      }
      
      input.clr-color {
        color: #444;
        text-align: center;
        box-shadow: none;
        background-color: #fff;
        border: 1px solid #ddd;
        border-radius: 16px;
        order: 1;
        width: calc(70%);
        height: 32px;
        margin: 0 0px 10px 0px;
        padding: 0 8px;
        font-size: 12px;
        font-weight: bold;
      }
      
      input.clr-color:focus {
        border: 1px solid #1e90ff;
        outline: none;
      }
      
      .clr-close, .clr-clear {
        color: #fff;
        cursor: pointer;
        background-color: #666;
        border: 0;
        border-radius: 12px;
        order: 2;
        height: 24px;
        margin: 0 20px 20px;
        padding: 0 20px;
        font-family: inherit;
        font-size: 12px;
        font-weight: 400;
        display: none;
      }
      
      .clr-close {
        margin: 0 20px 20px auto;
        display: block;
      }
      
      .clr-preview {
        border-radius: 50%;
        width: 32px;
        height: 32px;
        margin: 0 0 0px 8px;
        position: relative;
        overflow: hidden;
        left: -6px;
      }
      
      .clr-preview:before, .clr-preview:after {
        content: "";
        border: 1px solid #fff;
        border-radius: 50%;
        width: 100%;
        height: 100%;
        position: absolute;
        top: 0;
        left: 0;
      }
      
      .clr-preview:after {
        background-color: currentColor;
        border: 0;
        box-shadow: inset 0 0 0 1px #0000001a;
      }
      
      .clr-preview button {
        z-index: 1;
        outline-offset: -2px;
        text-indent: -9999px;
        cursor: pointer;
        background-color: #0000;
        border: 0;
        border-radius: 50%;
        width: 100%;
        height: 100%;
        margin: 0;
        padding: 0;
        position: absolute;
        overflow: hidden;
      }
      
      .clr-marker, .clr-hue div, .clr-alpha div, .clr-color {
        box-sizing: border-box;
      }
      
      .clr-field {
        color: #0000;
        display: inline-block;
        position: relative;
      }
      
      .clr-field input {
        direction: ltr;
        margin: 0;
      }
      
      .clr-field.clr-rtl input {
        text-align: right;
      }
      
      .clr-field button {
        color: inherit;
        text-indent: -1000px;
        white-space: nowrap;
        pointer-events: none;
        border: 0;
        width: 30px;
        height: 100%;
        margin: 0;
        padding: 0;
        position: absolute;
        top: 50%;
        right: 0;
        overflow: hidden;
        transform: translateY(-50%);
      }
      
      .clr-field.clr-rtl button {
        left: 0;
        right: auto;
      }
      
      .clr-field button:after {
        content: "";
        border-radius: inherit;
        background-color: currentColor;
        width: 100%;
        height: 100%;
        display: block;
        position: absolute;
        top: 0;
        left: 0;
        box-shadow: inset 0 0 1px #00000080;
      }
      
      .clr-alpha, .clr-alpha div, .clr-swatches button, .clr-preview:before, .clr-field button {
        background-image: repeating-linear-gradient(45deg, #aaa 25%, #0000 25% 75%, #aaa 75%, #aaa), repeating-linear-gradient(45deg, #aaa 25%, #fff 25% 75%, #aaa 75%, #aaa);
        background-position: 0 0, 4px 4px;
        background-size: 8px 8px;
      }
      
      .clr-marker:focus {
        outline: none;
      }
      
      .clr-keyboard-nav .clr-marker:focus, .clr-keyboard-nav .clr-hue input:focus + div, .clr-keyboard-nav .clr-alpha input:focus + div, .clr-keyboard-nav .clr-segmented input:focus + label {
        outline: none;
        box-shadow: 0 0 0 2px #1e90ff, 0 0 2px 2px #fff;
      }
      
      .clr-picker[data-alpha="false"] .clr-alpha {
        display: none;
      }
      
      .clr-picker[data-minimal="true"] {
        padding-top: 16px;
      }
      
      .clr-picker[data-minimal="true"] .clr-gradient, .clr-picker[data-minimal="true"] .clr-hue, .clr-picker[data-minimal="true"] .clr-alpha, .clr-picker[data-minimal="true"] .clr-color, .clr-picker[data-minimal="true"] .clr-preview {
        display: none;
      }
      
      .clr-dark {
        background: none;
      }
      
      .clr-dark .clr-segmented {
        border-color: #777;
      }
      
      .clr-dark .clr-swatches button:after {
        box-shadow: inset 0 0 0 1px #ffffff4d;
      }
      
      .clr-dark input.clr-color {
        color: rgba(255,255,255,0.85);
        background: rgba(var(--color-theme),0.05);
        border-color: rgba(var(--color-theme),0.2);
      }
      
      .clr-dark input.clr-color:focus {
        border-color: #1e90ff;
      }
      
      .clr-dark .clr-preview:after {
        box-shadow: inset 0 0 0 1px #ffffff80;
      }
      
      .clr-dark .clr-alpha, .clr-dark .clr-alpha div, .clr-dark .clr-swatches button, .clr-dark .clr-preview:before {
        background-image: repeating-linear-gradient(45deg, #666 25%, #0000 25% 75%, #888 75%, #888), repeating-linear-gradient(45deg, #888 25%, #444 25% 75%, #888 75%, #888);
      }
      
      .clr-picker.clr-polaroid {
        border-radius: 6px;
        box-shadow: 0 0 5px #0000001a, 0 5px 30px #0003;
      }
      
      .clr-picker.clr-polaroid:before {
        content: "";
        box-sizing: border-box;
        color: #fff;
        filter: drop-shadow(0 -4px 3px #0000001a);
        pointer-events: none;
        border: 8px solid #0000;
        border-top-width: 0;
        border-bottom: 10px solid;
        width: 16px;
        height: 10px;
        display: block;
        position: absolute;
        top: -10px;
        left: 20px;
      }
      
      .clr-picker.clr-polaroid.clr-dark:before {
        color: #444;
      }
      
      .clr-picker.clr-polaroid.clr-left:before {
        left: auto;
        right: 20px;
      }
      
      .clr-picker.clr-polaroid.clr-top:before {
        top: auto;
        bottom: -10px;
        transform: rotateZ(180deg);
      }
      
      .clr-polaroid .clr-gradient {
        border-radius: 3px;
        width: calc(100% - 20px);
        height: 120px;
        margin: 10px;
      }
      
      .clr-polaroid .clr-hue, .clr-polaroid .clr-alpha {
        border-radius: 5px;
        width: calc(100% - 30px);
        height: 10px;
        margin: 6px 15px;
      }
      
      .clr-polaroid .clr-hue div, .clr-polaroid .clr-alpha div {
        box-shadow: 0 0 5px #0003;
      }
      
      .clr-polaroid .clr-format {
        width: calc(100% - 20px);
        margin: 0 10px 15px;
      }
      
      .clr-polaroid .clr-swatches {
        width: calc(100% - 12px);
        margin: 0 6px;
      }
      
      .clr-polaroid .clr-swatches div {
        padding-bottom: 10px;
      }
      
      .clr-polaroid .clr-swatches button {
        width: 22px;
        height: 22px;
      }
      
      .clr-polaroid input.clr-color {
        width: calc(100% - 60px);
        margin: 10px 10px 15px auto;
      }
      
      .clr-polaroid .clr-clear {
        margin: 0 10px 15px;
      }
      
      .clr-polaroid .clr-close {
        margin: 0 10px 15px auto;
      }
      
      .clr-polaroid .clr-preview {
        margin: 10px 0 15px 10px;
      }
      
      .clr-picker.clr-large {
        width: 275px;
      }
      
      .clr-large .clr-gradient {
        height: 150px;
      }
      
      .clr-large .clr-swatches button {
        width: 22px;
        height: 22px;
      }
      
      .clr-picker.clr-pill {
        box-sizing: border-box;
        width: 380px;
        padding-left: 180px;
      }
      
      .clr-pill .clr-gradient {
        border-radius: 3px 0 0 3px;
        width: 180px;
        height: 100%;
        margin-bottom: 0;
        position: absolute;
        top: 0;
        left: 0;
      }
      
      .clr-pill .clr-hue {
        margin-top: 20px;
      }            

      `}getCardSize(){return 1}}customElements.define("colorpicker-joy-card",o),console.info("%c ColorPicker Joy CARD \n%c Version 1.0 ","color: orange; font-weight: bold; background: black","color: white; font-weight: bold; background: dimgray");