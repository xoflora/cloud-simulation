uniform float exposure;
uniform float decay;
uniform float density;
uniform float weight;
uniform vec2 lightPositionOnScreen;
uniform float dotLightLook;
uniform sampler2D firstPass;
const int NUM_SAMPLES = 100;

void main() {
    gl_FragColor = vec4(0.0);
    
    if(dotLightLook < 0.0) {

        vec2 deltaTextCoord = vec2( gl_TexCoord[0].st - lightPositionOnScreen.xy);

        vec2 textCoo = gl_TexCoord[0].st;
        deltaTextCoord *= density /  float(NUM_SAMPLES);
        float illuminationDecay = 1.0;

        for(int i = 0; i < NUM_SAMPLES ; i++) {
            textCoo -= deltaTextCoord;
            vec4 sample = texture2D(firstPass, textCoo );

            if (sample.x > 0.992) {
                sample.x = 1.0;
            }
            if (sample.y > 0.992) {
                sample.y = 1.0;
            }
            if (sample.z > 0.992) {
                sample.z = 1.0;
            }
            if (sample.w > 0.992) {
                sample.w = 1.0;
            }

            sample.x = 1.-sample.x;
            sample.y = 1.-sample.y;
            sample.z = 1.-sample.z;
            sample.w = 1.-sample.w;

            sample *= (illuminationDecay * weight);
            gl_FragColor += sample;
            illuminationDecay *= decay;
        }

        gl_FragColor *= exposure;

    } else {
        gl_FragColor = vec4(1.0) - texture2D(firstPass, gl_TexCoord[0].st);
    }
}
