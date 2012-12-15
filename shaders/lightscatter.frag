uniform float exposure;
uniform float decay;
uniform float density;
uniform float weight;
uniform vec2 lightPositionOnScreen;
uniform sampler2D firstPass;
const int NUM_SAMPLES = 100;

void main() {
    gl_FragColor = vec4(0.0);
    
    vec2 deltaTextCoord = vec2( gl_TexCoord[0].st - lightPositionOnScreen.xy);
     
 //   deltaTextCoord.x = -abs(deltaTextCoord.x);
 //   deltaTextCoord.y = -abs(deltaTextCoord.y);
    vec2 textCoo = gl_TexCoord[0].st;
    deltaTextCoord *= density /  float(NUM_SAMPLES);
    float illuminationDecay = 1.0;

    for(int i = 0; i < NUM_SAMPLES ; i++) {
        textCoo -= deltaTextCoord;
        vec4 sample = texture2D(firstPass, textCoo );
	
	sample.x = 1.-sample.x;
	sample.y = 1.-sample.y;
	sample.z = 1.-sample.z;
	sample.w = 1.-sample.w;

	if (sample.x < 0.8) {
	    sample.x = 0.0;
	}
	if (sample.y < 0.8) {
	    sample.y = 0.0;
	}
	if (sample.z < 0.8) {
	    sample.z = 0.0;
	}
	if (sample.w < 0.8) {
	    sample.w = 0.0;
	}
//	sample.x = 1.0-sample.x;
//	sample.y = 1.0-sample.y;
//	sample.z = 1.0-sample.z;
//	sample.w = 1.0-sample.w;
        sample *= (illuminationDecay * weight);
        gl_FragColor += sample;
        illuminationDecay *= decay;
    }

    gl_FragColor *= exposure;
//    gl_FragColor = vec4(1.)-gl_FragColor;
}