const LitElement = customElements.get('home-assistant-main')
  ? Object.getPrototypeOf(customElements.get('home-assistant-main'))
  : Object.getPrototypeOf(customElements.get('hui-view'));
const html = LitElement.prototype.html;
const css = LitElement.prototype.css;
const helpers = await loadCardHelpers();

const version = '1.0.0';

class GridJoy extends LitElement {
  static get properties() {
    return {
      hass: {},
      config: {},
    }
  }


  setConfig(config) {
    if (!config.cards) {
      throw new Error("Cards must be specified");
    }

    this.config = deepClone(config);
    this.columns = this.config.columns ? this.config.columns : 3

    this.styles =  css`display: grid; grid-template-columns: repeat(${this.columns},minmax(0,1fr)); justify-content: center; align-items: center; grid-gap: var(--grid-card-gap,8px);
    `;

    this.cards = {};
    for (let k in this.config.cards) {
      this.cards[k] = helpers.createCardElement(this.config.cards[k]);
      this.cards[k].hass = this.hass;
    }

    window.addEventListener('resize', el => {
      try {
        this.shadowRoot.querySelector('#root').className = 'grid-joy';
      } catch (ex) {}
    });
  }




  updated(changedProperties) {
    if (changedProperties.has('hass')) {
      for (let k in this.cards) {
        this.cards[k].hass = this.hass;
      }
    }
  }

  render() {
    return html`
      <div id="root" class="gridjoy" style=${this.styles}>
        ${Object.keys(this.cards).map((k) =>
          html`
            ${this.cards[k]}
          `)}
      </div>
    `;
  }

  // firstUpdated() {
  //   this.shadowRoot.getElementById("root").style = this.styles;
  // }


}

if (!customElements.get('grid-joy-card')) {
  customElements.define('grid-joy-card', GridJoy);
  console.info(`%cGrid Joy ${version} IS INSTALLED`, "color: blue; font-weight: bold;");
}

function deepClone(value) {
  if (!(!!value && typeof value == 'object')) {
    return value;
  }
  if (Object.prototype.toString.call(value) == '[object Date]') {
    return new Date(value.getTime());
  }
  if (Array.isArray(value)) {
    return value.map(deepClone);
  }
  var result = {};
  Object.keys(value).forEach(
    function(key) { result[key] = deepClone(value[key]); });
  return result;
}