#include "Drawable.h"

Drawable::Drawable(DXContext* p_context, RenderPipeline* p_pipeline) : context(p_context), renderPipeline(p_pipeline) {}

Drawable::Drawable(DXContext* p_context, MeshPipeline* p_pipeline) : context(p_context), meshPipeline(p_pipeline) {}

void Drawable::constructScene() {}

void Drawable::draw(Camera* camera) {}

void Drawable::releaseResources() {}
