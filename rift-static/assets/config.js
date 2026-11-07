/* Rift website settings. Edit these, commit, and Cloudflare Pages redeploys.
 *
 * repo:   the GitHub repository whose Releases hold the app files ("owner/name").
 *         The download page asks GitHub for the latest release and links each
 *         button to the matching file. Until a release exists, buttons say so.
 * stores: browser extension store pages. Leave a value empty to show "Coming soon".
 */
window.RIFT_CONFIG = {
  repo: 'jeelnadaa/rift',
  stores: {
    chrome: '',   // https://chromewebstore.google.com/detail/rift/<id>
    edge: '',     // https://microsoftedge.microsoft.com/addons/detail/<id>
    firefox: '',  // https://addons.mozilla.org/firefox/addon/rift/
  },
};
