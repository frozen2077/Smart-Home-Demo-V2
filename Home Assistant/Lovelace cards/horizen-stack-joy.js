const LitElement = customElements.get('home-assistant-main')
  ? Object.getPrototypeOf(customElements.get('home-assistant-main'))
  : Object.getPrototypeOf(customElements.get('hui-view'));
const html = LitElement.prototype.html;
const css = LitElement.prototype.css;
const helpers = await loadCardHelpers();

const version = '1.0.0';

class HorizenStackJoy extends LitElement {
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

    this.config = {
      threshold: 800,
      ...config
    };

    this.cards = {};
    for (let k in config.cards) {
      this.cards[k] = helpers.createCardElement(config.cards[k]);
      this.cards[k].hass = this.hass;
    }

    window.addEventListener('resize', el => {
      try {
        this.shadowRoot.querySelector('#root').className = 'horizontal';
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
      <div id="root" class="horizontal">
        ${Object.keys(this.cards).map((k) =>
          html`
            ${this.cards[k]}
          `)}
      </div>
    `;
  }

  static get styles() {
    return css`
      #root { 
        display: flex; 
        justify-content: flex-start; 
        flex-wrap: wrap;
      }

      /* horizontal for wide devices */
      #root.horizontal > * { flex: 1 1 0; margin: 0 8px 0 0; }
      #root.horizontal > *:first-child { margin-left: 0; }
      #root.horizontal > *:last-child { margin-right: 0; }
    `;
  }
}

if (!customElements.get('horizen-stack-joy-card')) {
  customElements.define('horizen-stack-joy-card', HorizenStackJoy);
  console.info(`%cHorizen Stack Joy ${version} IS INSTALLED`, "color: blue; font-weight: bold;");
}
