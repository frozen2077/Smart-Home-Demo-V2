/**
 * @license
 * Copyright 2019 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */
const t=window.ShadowRoot&&(void 0===window.ShadyCSS||window.ShadyCSS.nativeShadow)&&"adoptedStyleSheets"in Document.prototype&&"replace"in CSSStyleSheet.prototype,i=Symbol(),s=new Map;class e{constructor(t,s){if(this._$cssResult$=!0,s!==i)throw Error("CSSResult is not constructable. Use `unsafeCSS` or `css` instead.");this.cssText=t}get styleSheet(){let i=s.get(this.cssText);return t&&void 0===i&&(s.set(this.cssText,i=new CSSStyleSheet),i.replaceSync(this.cssText)),i}toString(){return this.cssText}}const n=(t,...s)=>{const n=1===t.length?t[0]:s.reduce(((i,s,e)=>i+(t=>{if(!0===t._$cssResult$)return t.cssText;if("number"==typeof t)return t;throw Error("Value passed to 'css' function must be a 'css' function result: "+t+". Use 'unsafeCSS' to pass non-literal values, but take care to ensure page security.")})(s)+t[e+1]),t[0]);return new e(n,i)},o=t?t=>t:t=>t instanceof CSSStyleSheet?(t=>{let s="";for(const i of t.cssRules)s+=i.cssText;return(t=>new e("string"==typeof t?t:t+"",i))(s)})(t):t
/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */;var r;const l=window.trustedTypes,a=l?l.emptyScript:"",h=window.reactiveElementPolyfillSupport,d={toAttribute(t,i){switch(i){case Boolean:t=t?a:null;break;case Object:case Array:t=null==t?t:JSON.stringify(t)}return t},fromAttribute(t,i){let s=t;switch(i){case Boolean:s=null!==t;break;case Number:s=null===t?null:Number(t);break;case Object:case Array:try{s=JSON.parse(t)}catch(t){s=null}}return s}},c=(t,i)=>i!==t&&(i==i||t==t),u={attribute:!0,type:String,converter:d,reflect:!1,hasChanged:c};class v extends HTMLElement{constructor(){super(),this._$Et=new Map,this.isUpdatePending=!1,this.hasUpdated=!1,this._$Ei=null,this.o()}static addInitializer(t){var i;null!==(i=this.l)&&void 0!==i||(this.l=[]),this.l.push(t)}static get observedAttributes(){this.finalize();const t=[];return this.elementProperties.forEach(((i,s)=>{const e=this._$Eh(s,i);void 0!==e&&(this._$Eu.set(e,s),t.push(e))})),t}static createProperty(t,i=u){if(i.state&&(i.attribute=!1),this.finalize(),this.elementProperties.set(t,i),!i.noAccessor&&!this.prototype.hasOwnProperty(t)){const s="symbol"==typeof t?Symbol():"__"+t,e=this.getPropertyDescriptor(t,s,i);void 0!==e&&Object.defineProperty(this.prototype,t,e)}}static getPropertyDescriptor(t,i,s){return{get(){return this[i]},set(e){const n=this[t];this[i]=e,this.requestUpdate(t,n,s)},configurable:!0,enumerable:!0}}static getPropertyOptions(t){return this.elementProperties.get(t)||u}static finalize(){if(this.hasOwnProperty("finalized"))return!1;this.finalized=!0;const t=Object.getPrototypeOf(this);if(t.finalize(),this.elementProperties=new Map(t.elementProperties),this._$Eu=new Map,this.hasOwnProperty("properties")){const t=this.properties,i=[...Object.getOwnPropertyNames(t),...Object.getOwnPropertySymbols(t)];for(const s of i)this.createProperty(s,t[s])}return this.elementStyles=this.finalizeStyles(this.styles),!0}static finalizeStyles(t){const i=[];if(Array.isArray(t)){const s=new Set(t.flat(1/0).reverse());for(const t of s)i.unshift(o(t))}else void 0!==t&&i.push(o(t));return i}static _$Eh(t,i){const s=i.attribute;return!1===s?void 0:"string"==typeof s?s:"string"==typeof t?t.toLowerCase():void 0}o(){var t;this._$Ep=new Promise((t=>this.enableUpdating=t)),this._$AL=new Map,this._$Em(),this.requestUpdate(),null===(t=this.constructor.l)||void 0===t||t.forEach((t=>t(this)))}addController(t){var i,s;(null!==(i=this._$Eg)&&void 0!==i?i:this._$Eg=[]).push(t),void 0!==this.renderRoot&&this.isConnected&&(null===(s=t.hostConnected)||void 0===s||s.call(t))}removeController(t){var i;null===(i=this._$Eg)||void 0===i||i.splice(this._$Eg.indexOf(t)>>>0,1)}_$Em(){this.constructor.elementProperties.forEach(((t,i)=>{this.hasOwnProperty(i)&&(this._$Et.set(i,this[i]),delete this[i])}))}createRenderRoot(){var i;const s=null!==(i=this.shadowRoot)&&void 0!==i?i:this.attachShadow(this.constructor.shadowRootOptions);return((i,s)=>{t?i.adoptedStyleSheets=s.map((t=>t instanceof CSSStyleSheet?t:t.styleSheet)):s.forEach((t=>{const s=document.createElement("style"),e=window.litNonce;void 0!==e&&s.setAttribute("nonce",e),s.textContent=t.cssText,i.appendChild(s)}))})(s,this.constructor.elementStyles),s}connectedCallback(){var t;void 0===this.renderRoot&&(this.renderRoot=this.createRenderRoot()),this.enableUpdating(!0),null===(t=this._$Eg)||void 0===t||t.forEach((t=>{var i;return null===(i=t.hostConnected)||void 0===i?void 0:i.call(t)}))}enableUpdating(t){}disconnectedCallback(){var t;null===(t=this._$Eg)||void 0===t||t.forEach((t=>{var i;return null===(i=t.hostDisconnected)||void 0===i?void 0:i.call(t)}))}attributeChangedCallback(t,i,s){this._$AK(t,s)}_$ES(t,i,s=u){var e,n;const o=this.constructor._$Eh(t,s);if(void 0!==o&&!0===s.reflect){const r=(null!==(n=null===(e=s.converter)||void 0===e?void 0:e.toAttribute)&&void 0!==n?n:d.toAttribute)(i,s.type);this._$Ei=t,null==r?this.removeAttribute(o):this.setAttribute(o,r),this._$Ei=null}}_$AK(t,i){var s,e,n;const o=this.constructor,r=o._$Eu.get(t);if(void 0!==r&&this._$Ei!==r){const t=o.getPropertyOptions(r),l=t.converter,a=null!==(n=null!==(e=null===(s=l)||void 0===s?void 0:s.fromAttribute)&&void 0!==e?e:"function"==typeof l?l:null)&&void 0!==n?n:d.fromAttribute;this._$Ei=r,this[r]=a(i,t.type),this._$Ei=null}}requestUpdate(t,i,s){let e=!0;void 0!==t&&(((s=s||this.constructor.getPropertyOptions(t)).hasChanged||c)(this[t],i)?(this._$AL.has(t)||this._$AL.set(t,i),!0===s.reflect&&this._$Ei!==t&&(void 0===this._$EC&&(this._$EC=new Map),this._$EC.set(t,s))):e=!1),!this.isUpdatePending&&e&&(this._$Ep=this._$E_())}async _$E_(){this.isUpdatePending=!0;try{await this._$Ep}catch(t){Promise.reject(t)}const t=this.scheduleUpdate();return null!=t&&await t,!this.isUpdatePending}scheduleUpdate(){return this.performUpdate()}performUpdate(){var t;if(!this.isUpdatePending)return;this.hasUpdated,this._$Et&&(this._$Et.forEach(((t,i)=>this[i]=t)),this._$Et=void 0);let i=!1;const s=this._$AL;try{i=this.shouldUpdate(s),i?(this.willUpdate(s),null===(t=this._$Eg)||void 0===t||t.forEach((t=>{var i;return null===(i=t.hostUpdate)||void 0===i?void 0:i.call(t)})),this.update(s)):this._$EU()}catch(t){throw i=!1,this._$EU(),t}i&&this._$AE(s)}willUpdate(t){}_$AE(t){var i;null===(i=this._$Eg)||void 0===i||i.forEach((t=>{var i;return null===(i=t.hostUpdated)||void 0===i?void 0:i.call(t)})),this.hasUpdated||(this.hasUpdated=!0,this.firstUpdated(t)),this.updated(t)}_$EU(){this._$AL=new Map,this.isUpdatePending=!1}get updateComplete(){return this.getUpdateComplete()}getUpdateComplete(){return this._$Ep}shouldUpdate(t){return!0}update(t){void 0!==this._$EC&&(this._$EC.forEach(((t,i)=>this._$ES(i,this[i],t))),this._$EC=void 0),this._$EU()}updated(t){}firstUpdated(t){}}
/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */
var p;v.finalized=!0,v.elementProperties=new Map,v.elementStyles=[],v.shadowRootOptions={mode:"open"},null==h||h({ReactiveElement:v}),(null!==(r=globalThis.reactiveElementVersions)&&void 0!==r?r:globalThis.reactiveElementVersions=[]).push("1.3.2");const f=globalThis.trustedTypes,w=f?f.createPolicy("lit-html",{createHTML:t=>t}):void 0,g=`lit$${(Math.random()+"").slice(9)}$`,m="?"+g,b=`<${m}>`,y=document,$=(t="")=>y.createComment(t),x=t=>null===t||"object"!=typeof t&&"function"!=typeof t,_=Array.isArray,S=t=>{var i;return _(t)||"function"==typeof(null===(i=t)||void 0===i?void 0:i[Symbol.iterator])},k=/<(?:(!--|\/[^a-zA-Z])|(\/?[a-zA-Z][^>\s]*)|(\/?$))/g,C=/-->/g,A=/>/g,M=/>|[ 	\n\r](?:([^\s"'>=/]+)([ 	\n\r]*=[ 	\n\r]*(?:[^ 	\n\r"'`<>=]|("|')|))|$)/g,E=/'/g,O=/"/g,T=/^(?:script|style|textarea|title)$/i,U=(t=>(i,...s)=>({_$litType$:t,strings:i,values:s}))(1),j=Symbol.for("lit-noChange"),D=Symbol.for("lit-nothing"),z=new WeakMap,N=y.createTreeWalker(y,129,null,!1),P=(t,i)=>{const s=t.length-1,e=[];let n,o=2===i?"<svg>":"",r=k;for(let i=0;i<s;i++){const s=t[i];let l,a,h=-1,d=0;for(;d<s.length&&(r.lastIndex=d,a=r.exec(s),null!==a);)d=r.lastIndex,r===k?"!--"===a[1]?r=C:void 0!==a[1]?r=A:void 0!==a[2]?(T.test(a[2])&&(n=RegExp("</"+a[2],"g")),r=M):void 0!==a[3]&&(r=M):r===M?">"===a[0]?(r=null!=n?n:k,h=-1):void 0===a[1]?h=-2:(h=r.lastIndex-a[2].length,l=a[1],r=void 0===a[3]?M:'"'===a[3]?O:E):r===O||r===E?r=M:r===C||r===A?r=k:(r=M,n=void 0);const c=r===M&&t[i+1].startsWith("/>")?" ":"";o+=r===k?s+b:h>=0?(e.push(l),s.slice(0,h)+"$lit$"+s.slice(h)+g+c):s+g+(-2===h?(e.push(void 0),i):c)}const l=o+(t[s]||"<?>")+(2===i?"</svg>":"");if(!Array.isArray(t)||!t.hasOwnProperty("raw"))throw Error("invalid template strings array");return[void 0!==w?w.createHTML(l):l,e]};class B{constructor({strings:t,_$litType$:i},s){let e;this.parts=[];let n=0,o=0;const r=t.length-1,l=this.parts,[a,h]=P(t,i);if(this.el=B.createElement(a,s),N.currentNode=this.el.content,2===i){const t=this.el.content,i=t.firstChild;i.remove(),t.append(...i.childNodes)}for(;null!==(e=N.nextNode())&&l.length<r;){if(1===e.nodeType){if(e.hasAttributes()){const t=[];for(const i of e.getAttributeNames())if(i.endsWith("$lit$")||i.startsWith(g)){const s=h[o++];if(t.push(i),void 0!==s){const t=e.getAttribute(s.toLowerCase()+"$lit$").split(g),i=/([.?@])?(.*)/.exec(s);l.push({type:1,index:n,name:i[2],strings:t,ctor:"."===i[1]?F:"?"===i[1]?W:"@"===i[1]?Z:H})}else l.push({type:6,index:n})}for(const i of t)e.removeAttribute(i)}if(T.test(e.tagName)){const t=e.textContent.split(g),i=t.length-1;if(i>0){e.textContent=f?f.emptyScript:"";for(let s=0;s<i;s++)e.append(t[s],$()),N.nextNode(),l.push({type:2,index:++n});e.append(t[i],$())}}}else if(8===e.nodeType)if(e.data===m)l.push({type:2,index:n});else{let t=-1;for(;-1!==(t=e.data.indexOf(g,t+1));)l.push({type:7,index:n}),t+=g.length-1}n++}}static createElement(t,i){const s=y.createElement("template");return s.innerHTML=t,s}}function I(t,i,s=t,e){var n,o,r,l;if(i===j)return i;let a=void 0!==e?null===(n=s._$Cl)||void 0===n?void 0:n[e]:s._$Cu;const h=x(i)?void 0:i._$litDirective$;return(null==a?void 0:a.constructor)!==h&&(null===(o=null==a?void 0:a._$AO)||void 0===o||o.call(a,!1),void 0===h?a=void 0:(a=new h(t),a._$AT(t,s,e)),void 0!==e?(null!==(r=(l=s)._$Cl)&&void 0!==r?r:l._$Cl=[])[e]=a:s._$Cu=a),void 0!==a&&(i=I(t,a._$AS(t,i.values),a,e)),i}class R{constructor(t,i){this.v=[],this._$AN=void 0,this._$AD=t,this._$AM=i}get parentNode(){return this._$AM.parentNode}get _$AU(){return this._$AM._$AU}p(t){var i;const{el:{content:s},parts:e}=this._$AD,n=(null!==(i=null==t?void 0:t.creationScope)&&void 0!==i?i:y).importNode(s,!0);N.currentNode=n;let o=N.nextNode(),r=0,l=0,a=e[0];for(;void 0!==a;){if(r===a.index){let i;2===a.type?i=new L(o,o.nextSibling,this,t):1===a.type?i=new a.ctor(o,a.name,a.strings,this,t):6===a.type&&(i=new J(o,this,t)),this.v.push(i),a=e[++l]}r!==(null==a?void 0:a.index)&&(o=N.nextNode(),r++)}return n}m(t){let i=0;for(const s of this.v)void 0!==s&&(void 0!==s.strings?(s._$AI(t,s,i),i+=s.strings.length-2):s._$AI(t[i])),i++}}class L{constructor(t,i,s,e){var n;this.type=2,this._$AH=D,this._$AN=void 0,this._$AA=t,this._$AB=i,this._$AM=s,this.options=e,this._$Cg=null===(n=null==e?void 0:e.isConnected)||void 0===n||n}get _$AU(){var t,i;return null!==(i=null===(t=this._$AM)||void 0===t?void 0:t._$AU)&&void 0!==i?i:this._$Cg}get parentNode(){let t=this._$AA.parentNode;const i=this._$AM;return void 0!==i&&11===t.nodeType&&(t=i.parentNode),t}get startNode(){return this._$AA}get endNode(){return this._$AB}_$AI(t,i=this){t=I(this,t,i),x(t)?t===D||null==t||""===t?(this._$AH!==D&&this._$AR(),this._$AH=D):t!==this._$AH&&t!==j&&this.$(t):void 0!==t._$litType$?this.T(t):void 0!==t.nodeType?this.k(t):S(t)?this.S(t):this.$(t)}M(t,i=this._$AB){return this._$AA.parentNode.insertBefore(t,i)}k(t){this._$AH!==t&&(this._$AR(),this._$AH=this.M(t))}$(t){this._$AH!==D&&x(this._$AH)?this._$AA.nextSibling.data=t:this.k(y.createTextNode(t)),this._$AH=t}T(t){var i;const{values:s,_$litType$:e}=t,n="number"==typeof e?this._$AC(t):(void 0===e.el&&(e.el=B.createElement(e.h,this.options)),e);if((null===(i=this._$AH)||void 0===i?void 0:i._$AD)===n)this._$AH.m(s);else{const t=new R(n,this),i=t.p(this.options);t.m(s),this.k(i),this._$AH=t}}_$AC(t){let i=z.get(t.strings);return void 0===i&&z.set(t.strings,i=new B(t)),i}S(t){_(this._$AH)||(this._$AH=[],this._$AR());const i=this._$AH;let s,e=0;for(const n of t)e===i.length?i.push(s=new L(this.M($()),this.M($()),this,this.options)):s=i[e],s._$AI(n),e++;e<i.length&&(this._$AR(s&&s._$AB.nextSibling,e),i.length=e)}_$AR(t=this._$AA.nextSibling,i){var s;for(null===(s=this._$AP)||void 0===s||s.call(this,!1,!0,i);t&&t!==this._$AB;){const i=t.nextSibling;t.remove(),t=i}}setConnected(t){var i;void 0===this._$AM&&(this._$Cg=t,null===(i=this._$AP)||void 0===i||i.call(this,t))}}class H{constructor(t,i,s,e,n){this.type=1,this._$AH=D,this._$AN=void 0,this.element=t,this.name=i,this._$AM=e,this.options=n,s.length>2||""!==s[0]||""!==s[1]?(this._$AH=Array(s.length-1).fill(new String),this.strings=s):this._$AH=D}get tagName(){return this.element.tagName}get _$AU(){return this._$AM._$AU}_$AI(t,i=this,s,e){const n=this.strings;let o=!1;if(void 0===n)t=I(this,t,i,0),o=!x(t)||t!==this._$AH&&t!==j,o&&(this._$AH=t);else{const e=t;let r,l;for(t=n[0],r=0;r<n.length-1;r++)l=I(this,e[s+r],i,r),l===j&&(l=this._$AH[r]),o||(o=!x(l)||l!==this._$AH[r]),l===D?t=D:t!==D&&(t+=(null!=l?l:"")+n[r+1]),this._$AH[r]=l}o&&!e&&this.C(t)}C(t){t===D?this.element.removeAttribute(this.name):this.element.setAttribute(this.name,null!=t?t:"")}}class F extends H{constructor(){super(...arguments),this.type=3}C(t){this.element[this.name]=t===D?void 0:t}}const K=f?f.emptyScript:"";class W extends H{constructor(){super(...arguments),this.type=4}C(t){t&&t!==D?this.element.setAttribute(this.name,K):this.element.removeAttribute(this.name)}}class Z extends H{constructor(t,i,s,e,n){super(t,i,s,e,n),this.type=5}_$AI(t,i=this){var s;if((t=null!==(s=I(this,t,i,0))&&void 0!==s?s:D)===j)return;const e=this._$AH,n=t===D&&e!==D||t.capture!==e.capture||t.once!==e.once||t.passive!==e.passive,o=t!==D&&(e===D||n);n&&this.element.removeEventListener(this.name,this,e),o&&this.element.addEventListener(this.name,this,t),this._$AH=t}handleEvent(t){var i,s;"function"==typeof this._$AH?this._$AH.call(null!==(s=null===(i=this.options)||void 0===i?void 0:i.host)&&void 0!==s?s:this.element,t):this._$AH.handleEvent(t)}}class J{constructor(t,i,s){this.element=t,this.type=6,this._$AN=void 0,this._$AM=i,this.options=s}get _$AU(){return this._$AM._$AU}_$AI(t){I(this,t)}}const V={L:"$lit$",P:g,V:m,I:1,N:P,R,j:S,D:I,H:L,F:H,O:W,W:Z,B:F,Z:J},q=window.litHtmlPolyfillSupport;
/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */
var G,Y;null==q||q(B,L),(null!==(p=globalThis.litHtmlVersions)&&void 0!==p?p:globalThis.litHtmlVersions=[]).push("2.2.4");class Q extends v{constructor(){super(...arguments),this.renderOptions={host:this},this._$Dt=void 0}createRenderRoot(){var t,i;const s=super.createRenderRoot();return null!==(t=(i=this.renderOptions).renderBefore)&&void 0!==t||(i.renderBefore=s.firstChild),s}update(t){const i=this.render();this.hasUpdated||(this.renderOptions.isConnected=this.isConnected),super.update(t),this._$Dt=((t,i,s)=>{var e,n;const o=null!==(e=null==s?void 0:s.renderBefore)&&void 0!==e?e:i;let r=o._$litPart$;if(void 0===r){const t=null!==(n=null==s?void 0:s.renderBefore)&&void 0!==n?n:null;o._$litPart$=r=new L(i.insertBefore($(),t),t,void 0,null!=s?s:{})}return r._$AI(t),r})(i,this.renderRoot,this.renderOptions)}connectedCallback(){var t;super.connectedCallback(),null===(t=this._$Dt)||void 0===t||t.setConnected(!0)}disconnectedCallback(){var t;super.disconnectedCallback(),null===(t=this._$Dt)||void 0===t||t.setConnected(!1)}render(){return j}}Q.finalized=!0,Q._$litElement$=!0,null===(G=globalThis.litElementHydrateSupport)||void 0===G||G.call(globalThis,{LitElement:Q});const X=globalThis.litElementPolyfillSupport;null==X||X({LitElement:Q}),(null!==(Y=globalThis.litElementVersions)&&void 0!==Y?Y:globalThis.litElementVersions=[]).push("3.2.0");
/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */
const tt=t=>i=>"function"==typeof i?((t,i)=>(window.customElements.define(t,i),i))(t,i):((t,i)=>{const{kind:s,elements:e}=i;return{kind:s,elements:e,finisher(i){window.customElements.define(t,i)}}})(t,i)
/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */,it=(t,i)=>"method"===i.kind&&i.descriptor&&!("value"in i.descriptor)?{...i,finisher(s){s.createProperty(i.key,t)}}:{kind:"field",key:Symbol(),placement:"own",descriptor:{},originalKey:i.key,initializer(){"function"==typeof i.initializer&&(this[i.key]=i.initializer.call(this))},finisher(s){s.createProperty(i.key,t)}};function st(t){return(i,s)=>void 0!==s?((t,i,s)=>{i.constructor.createProperty(s,t)})(t,i,s):it(t,i)
/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */}function et(t){return st({...t,state:!0})}
/**
 * @license
 * Copyright 2021 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */var nt;null===(nt=window.HTMLSlotElement)||void 0===nt||nt.prototype.assignedElements;var ot=function(t,i,s,e){for(var n,o=arguments.length,r=o<3?i:null===e?e=Object.getOwnPropertyDescriptor(i,s):e,l=t.length-1;l>=0;l--)(n=t[l])&&(r=(o<3?n(r):o>3?n(i,s,r):n(i,s))||r);return o>3&&r&&Object.defineProperty(i,s,r),r};let rt=class extends Q{constructor(){super(...arguments),this.hass=void 0,this.config=void 0,this.entries=[],this.loadingEntries=!0}setConfig(t){this.config=t}setServer(t){this.setConfigProperty("entry_id",t.target.value);const i=new Event("config-changed",{bubbles:!0,composed:!0});i.detail={config:this.config},this.dispatchEvent(i)}setConfigProperty(t,i){this.config={...this.config,[t]:i}}stopPropagation(t){t.stopPropagation()}render(){return this.hass?U`
      <div class="card-config">
        ${this.renderServers()}
      </div>
    `:D}renderServers(){if(this.loadingEntries)return U`<span>loading available aria servers</span>`;if(0===this.entries.length)return U`<span>No aria2 integration found. Follow this <a target="_blank" href="https://github.com/deblockt/hass-aria2#installation">documentation</a> </span>`;const t=this.config.entry_id||this.entries[0].entry_id;return U`
      <ha-select
          label="server"
          @selected=${this.setServer}
          @closed=${this.stopPropagation}
          .value=${t}
          fixedMenuPosition
          naturalMenuWidth
        >
            ${this.entries.map((t=>U` <mwc-list-item .value=${t.entry_id}>${t.title}</mwc-list-item>`))}
        </ha-select>
    `}async firstUpdated(){this.loadingEntries=!0,this.entries=(await this.hass.callWS({type:"config_entries/get",domain:"aria2"})).filter((t=>"loaded"===t.state)),this.loadingEntries=!1}};ot([st()],rt.prototype,"hass",void 0),ot([st()],rt.prototype,"config",void 0),ot([et()],rt.prototype,"entries",void 0),ot([et()],rt.prototype,"loadingEntries",void 0),rt=ot([tt("aria2-card-editor")],rt),window.customCards=window.customCards||[],window.customCards.push({type:"aria2-card",name:"Aria2 Card",preview:!1,description:"A card to manage aria2 download server"});
/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */
const lt=2;
/**
 * @license
 * Copyright 2020 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */
const{H:at}=V,ht=()=>document.createComment(""),dt=(t,i,s)=>{var e;const n=t._$AA.parentNode,o=void 0===i?t._$AB:i._$AA;if(void 0===s){const i=n.insertBefore(ht(),o),e=n.insertBefore(ht(),o);s=new at(i,e,t,t.options)}else{const i=s._$AB.nextSibling,r=s._$AM,l=r!==t;if(l){let i;null===(e=s._$AQ)||void 0===e||e.call(s,t),s._$AM=t,void 0!==s._$AP&&(i=t._$AU)!==r._$AU&&s._$AP(i)}if(i!==o||l){let t=s._$AA;for(;t!==i;){const i=t.nextSibling;n.insertBefore(t,o),t=i}}}return s},ct=(t,i,s=t)=>(t._$AI(i,s),t),ut={},vt=t=>{var i;null===(i=t._$AP)||void 0===i||i.call(t,!1,!0);let s=t._$AA;const e=t._$AB.nextSibling;for(;s!==e;){const t=s.nextSibling;s.remove(),s=t}},pt=(t,i,s)=>{const e=new Map;for(let n=i;n<=s;n++)e.set(t[n],n);return e},ft=(t=>(...i)=>({_$litDirective$:t,values:i}))(class extends class{constructor(t){}get _$AU(){return this._$AM._$AU}_$AT(t,i,s){this._$Ct=t,this._$AM=i,this._$Ci=s}_$AS(t,i){return this.update(t,i)}update(t,i){return this.render(...i)}}{constructor(t){if(super(t),t.type!==lt)throw Error("repeat() can only be used in text expressions")}dt(t,i,s){let e;void 0===s?s=i:void 0!==i&&(e=i);const n=[],o=[];let r=0;for(const i of t)n[r]=e?e(i,r):r,o[r]=s(i,r),r++;return{values:o,keys:n}}render(t,i,s){return this.dt(t,i,s).values}update(t,[i,s,e]){var n;const o=(t=>t._$AH)(t),{values:r,keys:l}=this.dt(i,s,e);if(!Array.isArray(o))return this.ut=l,r;const a=null!==(n=this.ut)&&void 0!==n?n:this.ut=[],h=[];let d,c,u=0,v=o.length-1,p=0,f=r.length-1;for(;u<=v&&p<=f;)if(null===o[u])u++;else if(null===o[v])v--;else if(a[u]===l[p])h[p]=ct(o[u],r[p]),u++,p++;else if(a[v]===l[f])h[f]=ct(o[v],r[f]),v--,f--;else if(a[u]===l[f])h[f]=ct(o[u],r[f]),dt(t,h[f+1],o[u]),u++,f--;else if(a[v]===l[p])h[p]=ct(o[v],r[p]),dt(t,o[u],o[v]),v--,p++;else if(void 0===d&&(d=pt(l,p,f),c=pt(a,u,v)),d.has(a[u]))if(d.has(a[v])){const i=c.get(l[p]),s=void 0!==i?o[i]:null;if(null===s){const i=dt(t,o[u]);ct(i,r[p]),h[p]=i}else h[p]=ct(s,r[p]),dt(t,o[u],s),o[i]=null;p++}else vt(o[v]),v--;else vt(o[u]),u++;for(;p<=f;){const i=dt(t,h[f+1]);ct(i,r[p]),h[p++]=i}for(;u<=v;){const t=o[u++];null!==t&&vt(t)}return this.ut=l,((t,i=ut)=>{t._$AH=i})(t,h),j}});
/**
 * @license
 * Copyright 2017 Google LLC
 * SPDX-License-Identifier: BSD-3-Clause
 */
 function formatSize(t){const i=["B","KB","MB","GB"];let s=i[0];t||(t=0);for(var e=1;e<i.length&&t>=1024;e++)t/=1024,s=i[e];return t.toFixed(2)+" "+s}
 function wt(t){return 100*t.completed_length/t.total_length}
 var gt=function(t,i,s,e){for(var n,o=arguments.length,r=o<3?i:null===e?e=Object.getOwnPropertyDescriptor(i,s):e,l=t.length-1;l>=0;l--)(n=t[l])&&(r=(o<3?n(r):o>3?n(i,s,r):n(i,s))||r);return o>3&&r&&Object.defineProperty(i,s,r),r};
 let mt=class extends Q{constructor(){super(...arguments),this.hass=void 0,this.config=void 0,this.downloads=[]}
 render(){return this.config&&this.hass?"entry_id"in this.config?U`
      <ha-card>
        <div class="card-content">
          <div class="start-download-row" style="padding-bottom: 12px; justify-content: space-between">
            <ha-textfield
			  id="bturl-input"
		      icontrailing
              placeholder="url of the file to download"
              @keydown=${this.startDownloadOnEnter}
			  style="flex-basis: 75%; height: 44px;"
            ></ha-textfield>
            <ha-icon-button @click="${this.startDownload}" style="background-color: rgba(var(--color-theme),0.05); border-radius: 14px; margin: 0 2px; height: 44px; --mdc-icon-button-size: 44px">
              <ha-icon style="display: flex" icon="hass:play"></ha-icon>
            </ha-icon-button>
          </div>
          <div class="download-list">
            ${ft((this.downloads||[]).filter((t) => t.name.indexOf("METADATA")=="-1"&&t.status=="complete").concat((this.downloads||[]).filter((t) => t.name.indexOf("METADATA")=="-1"&&t.status!="complete").reverse()).reverse().slice(0,5),(t=>t.gid),(t=>U`
              <div class="item" style='display:grid; grid-template-areas: "h d" "b c" "s c"; grid-template-columns: 3fr 1fr; grid-template-rows: 30% 20% auto; margin-top: 12px;'>
				<span @click="${()=>this.openDetail(t)}" class="name" style="padding-left:0px; grid-area: h;">${t.name}</span>
                ${this.buildProgressBar(t)}		
				${this.buildSpeed(t)}
				${this.buildTextProgress(t)}
                ${this.buildDownloadIcon(t)}			
              </div>
            `))}
          </div>
        </div>
      </ha-card>
    `:U`<ha-card> <div class="card-content"> You should edit the card to select the aria server to use. </div> </ha-card>`:U``}
	openDetail(t){const i=document.getElementsByTagName("home-assistant")[0].shadowRoot,s=null==i?void 0:i.querySelector("download-detail-dialog");if(s)s.currentDownload=t;else{const s=document.createElement("download-detail-dialog");s.currentDownload=t,null==i||i.appendChild(s)}}
	buildDownloadIcon(t){return"complete"===t.status||"removed"===t.status?U`
	<div id="control" style="grid-area: c; display:flex; flex-direction:row; justify-content: flex-end; --mdc-icon-button-size: 44px">
	  <ha-icon-button disabled="disabled" style="background-color: rgba(var(--color-theme),0.05); border-radius: 14px; margin: 4px 0 0 6px; height: 44px; padding: 1px 0 0 0"><ha-icon style="display: flex" icon="hass:check"></ha-icon></ha-icon-button>
	</div>
	  `:"paused"===t.status?U`
	  <div id="control" style="grid-area: c; display:flex; flex-direction:row; justify-content: space-evenly; --mdc-icon-button-size: 44px">
        <ha-icon-button @click="${()=>this.actionOnDownload("remove",t)}" style="background-color: rgba(var(--color-theme),0.05); border-radius: 14px; margin: 4px 10px 0 6px;  height: 44px; padding: 1px 0 0 0"><ha-icon style="display: flex;" icon="hass:stop" ></ha-icon></ha-icon-button>	  
        <ha-icon-button @click="${()=>this.actionOnDownload("resume",t)}" style="background-color: rgba(var(--color-theme),0.05); border-radius: 14px; margin: 4px 0 0 0; height: 44px; padding: 1px 0 0 0"><ha-icon style="display: flex;" icon="hass:play"></ha-icon></ha-icon-button>
	  </div>
      `:U`
	  <div id="control" style="grid-area: c; display:flex; flex-direction:row; justify-content: space-evenly; --mdc-icon-button-size: 44px">
        <ha-icon-button @click="${()=>this.actionOnDownload("remove",t)}" style="background-color: rgba(var(--color-theme),0.05); border-radius: 14px; margin: 4px 10px 0 6px; height: 44px; padding: 1px 0 0 0"><ha-icon style="display: flex;" icon="hass:stop" ></ha-icon></ha-icon-button>	  
        <ha-icon-button @click="${()=>this.actionOnDownload("pause",t)}" style="background-color: rgba(var(--color-theme),0.05); border-radius: 14px; margin: 4px 0 0 0; height: 44px; padding: 1px 0 0 0"><ha-icon style="display: flex;" icon="hass:pause"></ha-icon></ha-icon-button>
      </div>		
      `}
	  buildProgressBar(t){return"complete"!=t.status&&"removed"!==t.status?U`
        <div class="progress" style="width: ${wt(t)}%; height: 2px; padding: 0 0 4px 0; position: relative; top: 0; border-radius: 14px; background-color: var(--primary-color); grid-area: b; place-self: center start">
        </div>
      `:U`
        <div class="progress" style="width: 100%; height: 2px; padding: 0 0 4px 0; position: relative; top: 0; border-radius: 14px; background-color: var(--primary-color); grid-area: b; place-self: center start">
        </div>	  
	  `}
	  buildSpeed(t){return"complete"!=t.status&&"removed"!==t.status?U`
		<div id="speed" style="grid-area: s; display:flex; align-items: center;">
			<span style="background-color: rgba(var(--color-theme),0.05); border-radius: 8px; text-align: center; padding: 4px; margin: 4px 4px 0 0; flex-grow: 1; font-weight: bold; font-size: small; flex-basis: 50%;">${formatSize(t.download_speed)}/s</span>
			<span style="background-color: rgba(var(--color-theme),0.05); border-radius: 8px; text-align: center; padding: 4px; margin: 4px 4px 0 0; flex-grow: 1; font-weight: bold; font-size: small; flex-basis: 50%;">${formatSize(t.total_length)}</span>
		</div>	
      `:U`
		<div id="speed" style="grid-area: s; display:flex; align-items: center;">
			<span style="background-color: rgba(var(--color-theme),0.05); border-radius: 8px; text-align: center; padding: 4px; margin: 4px 4px 0 0; flex-grow: 1; font-weight: bold; font-size: small; flex-basis: 50%;">0B/s</span>
			<span style="background-color: rgba(var(--color-theme),0.05); border-radius: 8px; text-align: center; padding: 4px; margin: 4px 4px 0 0; flex-grow: 1; font-weight: bold; font-size: small; flex-basis: 50%;">${formatSize(t.total_length)}</span>
		</div>		  
	  `}	  
	  buildTextProgress(t){return"complete"!=t.status&&"removed"!==t.status?U`
		<div id="text_progress" style="grid-area: d; display:flex; align-items: center;">
			<span style="background-color: transparent; border-radius: 8px; text-align: end; padding: 2px; margin: 0; flex-grow: 1; font-weight: bold; font-size: larger">${wt(t).toFixed(1)}%</span>
		</div>	
      `:U`
		<div id="text_progress" style="grid-area: d; display:flex; align-items: center;">
			<span style="background-color: transparent; border-radius: 8px; text-align: end; padding: 2px; margin: 0; flex-grow: 1; font-weight: bold; font-size: larger">100%</span>
		</div>		  
	  `}		  
	  get _downloadItem(){var t;return null===(t=this.shadowRoot)||void 0===t?void 0:t.querySelector("#bturl-input")}
	  async startDownloadOnEnter(t){13===t.keyCode&&this.startDownload()}
	  async startDownload(){this._downloadItem&&this._downloadItem.value&&this._downloadItem.value.length>0&&(await this.hass.callService("aria2","start_download",{url:this._downloadItem.value,server_entry_id:this.entryId}),this._downloadItem.value="")}
	  async actionOnDownload(t,i){await this.hass.callService("aria2",t+"_download",{gid:i.gid,server_entry_id:this.entryId})}
	  async _refresh(t){if(t.server_entry_id&&this.entryId!==t.server_entry_id)return;this.downloads=t.list;const i=document.getElementsByTagName("home-assistant")[0].shadowRoot,s=null==i?void 0:i.querySelector("download-detail-dialog");if(null==s?void 0:s.currentDownload){const t=this.downloads.filter((t=>{var i;return t.gid==(null===(i=null==s?void 0:s.currentDownload)||void 0===i?void 0:i.gid)}));t.length>0&&(s.currentDownload=t[0])}}
	  get entryId(){return this.config.entry_id}
	  setConfig(t){this.config=t,this.entryId&&this.hass&&(this.downloads=[],this.hass.callService("aria2","refresh_downloads",{server_entry_id:this.entryId}))}
	  firstUpdated(){this.hass.connection.subscribeEvents((t=>this._refresh(t.data)),"download_list_updated"),this.entryId&&this.hass.callService("aria2","refresh_downloads",{server_entry_id:this.entryId})}getCardSize(){return 1+Math.min(10,this.downloads.length)}
	  static getConfigElement(){return document.createElement("aria2-card-editor")}};
	  mt.styles=n`
    .start-download-row {
      display: flex;
      flex-direction: row;
      align-items: center;
    }

    .start-download-row > paper-input {
      flex-grow: 1;
    }

    .download-list .item {
      width: 100%;
      position: relative;
    }

    .download-list .info {
      display: flex;
      flex-direction: row;
      align-items: center;
      z-index: 1100;
      position: relative
    }

    .progress {
      height: 4px;
      position: relative;
      z-index: 1000;
      top: 0;
      background-color: var(--mdc-select-fill-color);
      transition: width 3000ms linear;
    }

    .download-list .item .name {
      flex-grow: 1;
      overflow: hidden;
      cursor: pointer;
      padding-left: 5px;
	  display: -webkit-box;
	  -webkit-line-clamp: 1;
	  -webkit-box-orient: vertical;
	  font-size: small;
	  font-weight: bold;
	  align-self: flex-start;
    }
  `,gt([st()],mt.prototype,"hass",void 0),gt([st()],mt.prototype,"config",void 0),gt([et()],mt.prototype,"downloads",void 0),mt=gt([tt("aria2-card")],mt);var bt=function(t,i,s,e){for(var n,o=arguments.length,r=o<3?i:null===e?e=Object.getOwnPropertyDescriptor(i,s):e,l=t.length-1;l>=0;l--)(n=t[l])&&(r=(o<3?n(r):o>3?n(i,s,r):n(i,s))||r);return o>3&&r&&Object.defineProperty(i,s,r),r};let yt=class extends Q{constructor(){super(...arguments),this.currentDownload=void 0}
  render(){if(!this.currentDownload)return U``;
  let t=U``;
  return"complete"!==this.currentDownload.status&&"paused"!==this.currentDownload.status&&(t=U`
        <span class="label">download speed: </span> <span class="value"> ${this.formatSize(this.currentDownload.download_speed)}/s </span> <br/>
        <span class="label">progress: </span> <span class="value"> ${this.buildProgress(this.currentDownload)} </span> <br/>
        <span class="label">remaining time: </span> <span class="value"> ${this.buildRemainingTime(this.currentDownload)} </span> <br/>
      `),U`
      <ha-dialog open hideactions heading=${this.currentDownload.name} @closed=${this.closeDetail}>
        <div>
          <span class="label">file: </span> <span class="value">${this.currentDownload.name}</span> <br/>
          <span class="label">status: </span> <span class="value">${this.currentDownload.status}</span><br/>
          <span class="label">size: </span> <span class="value"> ${this.formatSize(this.currentDownload.total_length)} </span> <br/>
          ${t}
        </div>

        <ha-header-bar slot="heading">
          <ha-icon-button slot="navigationIcon" dialogAction="cancel">
            <ha-icon style="display: flex" icon="mdi:close"></ha-icon>
          </ha-icon-button>
          <div slot="title" class="main-title">
            ${this.currentDownload.name}
          </div>
        </ha-header-bar>
      </ha-dialog>
      `}
	  
	  updated(t){t.has("currentDownload")&&null!=this.currentDownload&&window.history.pushState({currentDownload:this.currentDownload.gid},"")}
	  buildProgress(t){return wt(t).toFixed(2)+"% ("+this.formatSize(t.completed_length)+" of "+this.formatSize(t.total_length)+")"}
	  buildRemainingTime(t){const i=function(t){return(t.total_length-t.completed_length)/t.download_speed}(t);if(!isFinite(i))return"infinity";const s=Math.floor(i/86400),e=Math.floor(i%86400/3600),n=Math.floor(i%3600/60),o=Math.floor(i%60);let r="";return s>0&&(r+=s+" d "),e>0&&(r+=e+" h "),n>0&&(r+=n+" m "),0===e&&0===s&&(r+=o+" s"),r}
	  formatSize(t){const i=["B","KB","MB","GB"];let s=i[0];t||(t=0);for(var e=1;e<i.length&&t>=1024;e++)t/=1024,s=i[e];return t.toFixed(2)+" "+s}
	  closeDetail(){var t;void 0!==this.currentDownload&&window.history.state&&window.history.state.currentDownload===(null===(t=this.currentDownload)||void 0===t?void 0:t.gid)&&window.history.back(),this.currentDownload=void 0}
	  connectedCallback(){super.connectedCallback(),window.addEventListener("popstate",(()=>this.closeDetail()))}};yt.styles=n`
    .label {
      color: var(--secondary-text-color);
    }
    .value {
      color: var(--primary-text-color);
    }

    ha-header-bar {
      --mdc-theme-on-primary: var(--primary-text-color);
      --mdc-theme-primary: var(--mdc-theme-surface);
      flex-shrink: 0;
      display: block;
      border-bottom: 1px solid var(--mdc-dialog-scroll-divider-color, rgba(0, 0, 0, 0.12));
    }

    @media (max-width: 450px), (max-height: 500px) {
      ha-dialog {
          --mdc-dialog-min-width: calc( 100vw - env(safe-area-inset-right) - env(safe-area-inset-left) );
          --mdc-dialog-max-width: calc( 100vw - env(safe-area-inset-right) - env(safe-area-inset-left) );
          --mdc-dialog-min-height: 100%;
          --mdc-dialog-max-height: 100%;
          --vertial-align-dialog: flex-end;
          --ha-dialog-border-radius: 0px;
      }

      ha-header-bar {
        --mdc-theme-primary: var(--app-header-background-color);
        --mdc-theme-on-primary: var(--app-header-text-color, white);
      }
    }
  `,bt([st()],yt.prototype,"currentDownload",void 0),yt=bt([tt("download-detail-dialog")],yt);
