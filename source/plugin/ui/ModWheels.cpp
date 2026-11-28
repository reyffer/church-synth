#include "ModWheels.h"

#include "Theme.h"

namespace ui
{
namespace
{
constexpr float kWheel = 30.0f;
constexpr float kGap = 8.0f;
constexpr int kPanelH = 90;
constexpr uint32_t kOpenDelayMs = 500;
constexpr uint32_t kCloseGraceMs = 270;
constexpr float kSpan = juce::MathConstants<float>::twoPi * (300.0f / 360.0f); // the track: +-100 % reach its two ends

ModTarget* asTarget (juce::Component* c)
{
    auto* t = dynamic_cast<ModTarget*> (c);
    return t != nullptr && t->modDest() >= 0 ? t : nullptr;
}

juce::String percent (float a)
{
    const int p = juce::roundToInt (a * 100.0f);
    return (p > 0 ? "+" : p < 0 ? juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92")) : juce::String()) + juce::String (std::abs (p)) + " %";
}
} // namespace

ModWheels::ModWheels (UiContext& c) : ctx (c)
{
    setInterceptsMouseClicks (true, false);
    setAlwaysOnTop (true);
}

ModTarget* ModWheels::modTarget() const
{
    return asTarget (target.getComponent());
}

int ModWheels::activeSource() const noexcept
{
    return dragSrc >= 0 ? dragSrc : hoverSrc >= 0 ? hoverSrc : clickedSrc;
}

const ModWheels::Wheel* ModWheels::findWheel (int src) const
{
    for (const auto& w : wheels)
        if (w.src == src)
            return &w;
    return nullptr;
}

void ModWheels::open (juce::Component& t, int activeSrc)
{
    if (asTarget (&t) == nullptr)
        return;
    if (target.getComponent() != &t)
        repaint(); // the old panel and lines go
    target = &t;
    candidate = nullptr;
    leftAt = 0;
    hoverSrc = -1;
    clickedSrc = activeSrc;
    wheels.clear();
    rebuild();
    repaint();
}

void ModWheels::close()
{
    if (target == nullptr && wheels.empty())
        return;
    if (wheelEditSrc >= 0)
    {
        if (const auto* w = findWheel (wheelEditSrc))
            setAmount (w->src, w->amount, true);
        wheelEditSrc = -1;
    }
    target = nullptr;
    wheels.clear();
    hoverSrc = clickedSrc = dragSrc = -1;
    panel = {};
    repaint();
}

void ModWheels::dismiss()
{
    dismissedOn = target.getComponent();
    close();
}

void ModWheels::rebuild()
{
    auto* t = modTarget();
    if (t == nullptr || ! visibleInEditor (*target) || getParentComponent() == nullptr)
    {
        close();
        return;
    }
    const auto routes = ctx.routesTo (t->modDest());
    if (routes.empty())
    {
        close();
        return;
    }
    bool same = routes.size() == wheels.size();
    for (size_t i = 0; same && i < routes.size(); ++i)
        same = wheels[i].src == routes[i].first;
    if (! same)
    {
        wheels.clear();
        for (const auto& [src, amt] : routes)
            wheels.push_back ({ src, amt, {} });
        if (findWheel (clickedSrc) == nullptr)
            clickedSrc = -1;
        if (findWheel (hoverSrc) == nullptr)
            hoverSrc = -1;
    }
    else
        for (size_t i = 0; i < routes.size(); ++i)
            wheels[i].amount = routes[i].second;
    layout();
}

void ModWheels::layout()
{
    auto* t = modTarget();
    if (t == nullptr)
        return;
    const auto dial = getLocalArea (target.getComponent(), t->modBounds());
    const int n = (int) wheels.size();
    const int w = juce::jmax (196, juce::roundToInt ((float) n * (kWheel + kGap) + 16.0f));
    int x = juce::roundToInt (dial.getRight()) + 8;
    if (x + w > getWidth() - 4)
        x = juce::roundToInt (dial.getX()) - 8 - w;
    x = juce::jlimit (4, juce::jmax (4, getWidth() - w - 4), x);
    int y = juce::roundToInt (dial.getCentreY() - 8.0f - kWheel * 0.5f);
    y = juce::jlimit (4, juce::jmax (4, getHeight() - kPanelH - 4), y);
    const juce::Rectangle<int> np (x, y, w, kPanelH);
    if (np != panel)
    {
        repaint(); // lines move too
        panel = np;
    }
    const float rowW = (float) n * kWheel + (float) (n - 1) * kGap;
    float wx = (float) panel.getCentreX() - rowW * 0.5f;
    for (auto& wh : wheels)
    {
        wh.r = { wx, (float) panel.getY() + 8.0f, kWheel, kWheel };
        wx += kWheel + kGap;
    }
}

int ModWheels::wheelAt (juce::Point<float> p) const
{
    for (const auto& w : wheels)
        if (w.r.expanded (4.0f).contains (p))
            return w.src;
    return -1;
}

void ModWheels::setAmount (int src, float amount, bool record)
{
    auto* t = modTarget();
    if (t == nullptr)
        return;
    amount = juce::jlimit (-1.0f, 1.0f, amount);
    ctx.setRouteAmount (src, t->modDest(), amount, record);
    for (auto& w : wheels)
        if (w.src == src)
            w.amount = amount;
    if (target != nullptr)
        target->repaint();
    repaint (panel);
}

void ModWheels::tick()
{
    const auto now = juce::Time::getMillisecondCounter();
    if (wheelEditSrc >= 0 && now - wheelEditAt > 400) // mouse-wheel nudges: one undo step
    {
        if (const auto* w = findWheel (wheelEditSrc))
            setAmount (w->src, w->amount, true);
        wheelEditSrc = -1;
    }
    if (blocked && blocked())
    {
        close();
        candidate = nullptr;
        return;
    }
    auto* root = getParentComponent();
    if (root == nullptr)
        return;

    // What is under the mouse: a modulated control, this panel, or something else.
    auto ms = juce::Desktop::getInstance().getMainMouseSource();
    auto* under = ms.getComponentUnderMouse();
    if (under != nullptr && under != root && ! root->isParentOf (under))
        under = nullptr;
    const bool overPanel = under == this;
    juce::Component* hovered = nullptr;
    for (auto* c = under; c != nullptr && c != root && ! overPanel; c = c->getParentComponent())
        if (asTarget (c) != nullptr)
        {
            hovered = c;
            break;
        }
    if (hovered != dismissedOn.getComponent())
        dismissedOn = nullptr;
    const bool buttonDown = juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown();
    const auto hasRoutes = [this] (juce::Component* c) { return c != nullptr && ! ctx.routesTo (asTarget (c)->modDest()).empty(); };

    if (isOpen())
    {
        if (overPanel || hovered == target.getComponent() || dragSrc >= 0)
        {
            leftAt = 0;
            candidate = nullptr;
        }
        else if (hovered != nullptr && hasRoutes (hovered) && hovered != dismissedOn.getComponent() && ! buttonDown)
        {
            if (candidate.getComponent() != hovered)
            {
                candidate = hovered;
                candidateSince = now;
            }
            else if (now - candidateSince >= kOpenDelayMs)
            {
                open (*hovered, -1);
                return;
            }
        }
        else
            candidate = nullptr;
        if (! overPanel && hovered != target.getComponent() && dragSrc < 0)
        {
            if (leftAt == 0)
                leftAt = now;
            else if (now - leftAt > kCloseGraceMs && candidate == nullptr)
            {
                close();
                return;
            }
        }
        rebuild();
        if (isOpen())
            repaint (panel); // live dots
        return;
    }

    if (hovered != nullptr && hovered != dismissedOn.getComponent() && ! buttonDown && hasRoutes (hovered))
    {
        if (candidate.getComponent() != hovered)
        {
            candidate = hovered;
            candidateSince = now;
        }
        else if (now - candidateSince >= kOpenDelayMs)
            open (*hovered, -1);
    }
    else
        candidate = nullptr;
}

// ---------------------------------------------------------------------------
void ModWheels::rangeTexts (const Wheel& w, juce::String& a, juce::String& b, bool& bipolar) const
{
    auto* t = modTarget();
    const auto& md = cs::modDestinations()[(size_t) t->modDest()];
    const float base = ctx.value (t->modParamId());
    const float nb = md.toNorm (base);
    bipolar = ctx.sourceBipolar (w.src);
    if (bipolar)
    {
        const float d = std::abs (w.amount);
        a = t->modValueText (md.fromNorm (juce::jlimit (0.0f, 1.0f, nb - d)));
        b = t->modValueText (md.fromNorm (juce::jlimit (0.0f, 1.0f, nb + d)));
    }
    else
    {
        a = t->modValueText (base);
        b = t->modValueText (md.fromNorm (juce::jlimit (0.0f, 1.0f, nb + w.amount)));
    }
}

juce::String ModWheels::rangeTextForTest() const
{
    const auto* w = findWheel (activeSource());
    if (w == nullptr || modTarget() == nullptr)
        return {};
    juce::String a, b;
    bool bip = false;
    rangeTexts (*w, a, b, bip);
    return juce::String (cs::modSourceName (w->src)) + " " + percent (w->amount) + " / " + a + (bip ? " <-> " : " -> ") + b;
}

void ModWheels::drawRange (juce::Graphics& g, juce::Rectangle<float> area, const Wheel& w) const
{
    juce::String a, b;
    bool bip = false;
    rangeTexts (w, a, b, bip);
    const auto f = font (12.0f, true);
    const float wa = juce::GlyphArrangement::getStringWidth (f, a);
    const float wb = juce::GlyphArrangement::getStringWidth (f, b);
    const float arrowW = 22.0f;
    const float total = wa + wb + arrowW + 12.0f;
    float x = area.getCentreX() - total * 0.5f;
    g.setFont (f);
    g.setColour (col::text);
    g.drawText (a, juce::Rectangle<float> (x, area.getY(), wa + 2.0f, area.getHeight()), juce::Justification::centredLeft);
    x += wa + 6.0f;
    const float cy = area.getCentreY();
    juce::Path arrow;
    arrow.addArrow ({ x, cy, x + arrowW, cy }, 1.4f, 7.0f, 6.0f);
    if (bip)
        arrow.addArrow ({ x + arrowW, cy, x, cy }, 1.4f, 7.0f, 6.0f);
    g.setColour (sourceColour (w.src));
    g.fillPath (arrow);
    x += arrowW + 6.0f;
    g.setColour (col::text);
    g.drawText (b, juce::Rectangle<float> (x, area.getY(), wb + 2.0f, area.getHeight()), juce::Justification::centredLeft);
}

void ModWheels::paint (juce::Graphics& g)
{
    if (! isOpen() || wheels.empty())
        return;
    const int active = activeSource();

    // Lines from the sources to the control itself (never to the panel: the panel is only a
    // read-out). Same path as the links overlay, so a source whose links are showing isn't drawn twice.
    auto* t = modTarget();
    if (active < 0 && tileBounds && t != nullptr)
    {
        const auto dial = getLocalArea (target.getComponent(), t->modBounds());
        const auto to = dial.getCentre();
        for (const auto& w : wheels)
        {
            const auto tile = tileBounds (w.src);
            if (tile.isEmpty() || w.src == ctx.selectedSource)
                continue;
            const juce::Point<float> from (tile.getCentreX(), to.y < tile.getY() ? tile.getY() : tile.getBottom());
            const float dy = (to.y - from.y) * 0.5f;
            juce::Path p;
            p.startNewSubPath (from);
            p.cubicTo (from.x, from.y + dy, to.x, to.y - dy, to.x, to.y);
            const auto c = sourceColour (w.src);
            g.setColour (c.withAlpha (0.6f));
            g.strokePath (p, juce::PathStrokeType (1.8f));
            g.setColour (c);
            g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre (from));
            g.drawRect (tile, 1.5f);
        }
    }

    const auto pr = panel.toFloat();
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillRoundedRectangle (pr.translated (0.0f, 3.0f), 7.0f);
    g.setColour (col::panelHi);
    g.fillRoundedRectangle (pr, 7.0f);
    // Outline in the connection's colour from the start (the hovered one, else the first).
    g.setColour (sourceColour (active >= 0 ? active : wheels.front().src).withAlpha (0.85f));
    g.drawRoundedRectangle (pr.reduced (0.5f), 7.0f, 1.4f);

    for (const auto& w : wheels)
    {
        const auto c = sourceColour (w.src);
        const auto ctr = w.r.getCentre();
        const float rad = kWheel * 0.5f - 2.5f;
        g.setColour (col::panelLo);
        g.fillEllipse (w.r);
        g.setColour (c.withAlpha (0.14f));
        g.fillEllipse (w.r);
        juce::Path track;
        track.addCentredArc (ctr.x, ctr.y, rad, rad, 0.0f, -kSpan * 0.5f, kSpan * 0.5f, true);
        g.setColour (col::border);
        g.strokePath (track, juce::PathStrokeType (2.0f));
        if (std::abs (w.amount) > 0.0005f)
        {
            juce::Path arc;
            arc.addCentredArc (ctr.x, ctr.y, rad, rad, 0.0f, 0.0f, w.amount * kSpan * 0.5f, true);
            g.setColour (c);
            g.strokePath (arc, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        // Live: where the source is pushing right now.
        const float live = w.amount * ctx.sourceValue (w.src) * kSpan * 0.5f;
        const juce::Point<float> dot (ctr.x + rad * std::sin (live), ctr.y - rad * std::cos (live));
        g.setColour (col::text);
        g.fillEllipse (juce::Rectangle<float> (4.0f, 4.0f).withCentre (dot));
        if (w.src == active)
        {
            g.setColour (c);
            g.drawEllipse (w.r.expanded (2.5f), 1.5f);
        }
    }

    auto info = pr.withTrimmedTop (8.0f + kWheel + 4.0f).reduced (8.0f, 2.0f);
    const auto* aw = findWheel (active);
    if (aw == nullptr)
    {
        g.setColour (col::textDim);
        g.setFont (font (11.0f));
        g.drawFittedText ("Hover a wheel to see its range.\nDrag it up/down to change the amount.", info.toNearestInt(), juce::Justification::centred, 2);
        return;
    }
    auto l1 = info.removeFromTop (info.getHeight() * 0.5f);
    g.setFont (font (12.0f, true));
    g.setColour (sourceColour (aw->src));
    g.drawText (juce::String (cs::modSourceName (aw->src)) + "   " + percent (aw->amount), l1, juce::Justification::centred);
    if (std::abs (aw->amount) < 0.0005f)
    {
        g.setColour (col::textDim);
        g.setFont (font (11.0f));
        g.drawText ("Drag the wheel up/down to set the amount", info, juce::Justification::centred);
    }
    else
        drawRange (g, info, *aw);
}

// ---------------------------------------------------------------------------
void ModWheels::mouseMove (const juce::MouseEvent& e)
{
    const int h = wheelAt (e.position);
    setMouseCursor (h >= 0 ? juce::MouseCursor::UpDownResizeCursor : juce::MouseCursor::NormalCursor);
    if (h != hoverSrc)
    {
        hoverSrc = h;
        repaint(); // lines hide / show
    }
}

void ModWheels::mouseExit (const juce::MouseEvent&)
{
    if (hoverSrc >= 0 && dragSrc < 0)
    {
        hoverSrc = -1;
        repaint();
    }
}

void ModWheels::mouseDown (const juce::MouseEvent& e)
{
    const int w = wheelAt (e.position);
    if (e.mods.isPopupMenu())
    {
        if (w >= 0)
            showMenu (w);
        return;
    }
    clickedSrc = w;
    if (w >= 0)
        if (const auto* wh = findWheel (w))
        {
            dragSrc = w;
            dragStart = wh->amount;
            dragMoved = false;
        }
    repaint();
}

void ModWheels::mouseDrag (const juce::MouseEvent& e)
{
    if (dragSrc < 0)
        return;
    const float scale = e.mods.isShiftDown() ? 800.0f : 160.0f;
    const float a = juce::jlimit (-1.0f, 1.0f, dragStart - (float) e.getDistanceFromDragStartY() / scale);
    if (const auto* w = findWheel (dragSrc); w != nullptr && std::abs (w->amount - a) > 1.0e-5f)
    {
        setAmount (dragSrc, a, false);
        dragMoved = true;
    }
}

void ModWheels::mouseUp (const juce::MouseEvent&)
{
    if (dragSrc >= 0 && dragMoved)
        if (const auto* w = findWheel (dragSrc))
            setAmount (w->src, w->amount, true); // one undo step for the whole drag
    dragSrc = -1;
    dragMoved = false;
}

void ModWheels::mouseDoubleClick (const juce::MouseEvent& e)
{
    const int w = wheelAt (e.position);
    if (auto* t = modTarget(); t != nullptr && w >= 0)
    {
        ctx.removeRoute (w, t->modDest());
        dragSrc = hoverSrc = clickedSrc = -1;
        if (target != nullptr)
            target->repaint();
        rebuild();
        repaint();
    }
}

void ModWheels::mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& d)
{
    const int w = wheelAt (e.position);
    const auto* wh = findWheel (w);
    if (wh == nullptr)
        return;
    const float delta = (d.isReversed ? -d.deltaY : d.deltaY) * (e.mods.isShiftDown() ? 0.025f : 0.1f);
    setAmount (w, wh->amount + delta, false);
    wheelEditSrc = w;
    wheelEditAt = juce::Time::getMillisecondCounter();
}

void ModWheels::showMenu (int src)
{
    juce::PopupMenu m;
    m.addSectionHeader (cs::modSourceName (src));
    m.addItem (1, "Remove connection");
    m.addItem (2, "Set amount to 0 %");
    m.addItem (3, "Invert amount");
    juce::Component::SafePointer<ModWheels> self (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (panel)),
                     [self, src] (int r) {
                         if (self == nullptr || r == 0)
                             return;
                         auto* t = self->modTarget();
                         const auto* w = self->findWheel (src);
                         if (t == nullptr || w == nullptr)
                             return;
                         if (r == 1)
                         {
                             self->ctx.removeRoute (src, t->modDest());
                             self->clickedSrc = self->hoverSrc = -1;
                             self->rebuild();
                         }
                         else if (r == 2)
                             self->setAmount (src, 0.0f, true);
                         else if (r == 3)
                             self->setAmount (src, -w->amount, true);
                         if (self != nullptr)
                             self->repaint();
                     });
}

} // namespace ui
