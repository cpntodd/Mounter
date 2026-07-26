/* discovery_page.cc — Network discovery with scan + mount */

#include "discovery_page.h"
#include "i18n.h"
#include "mount_page.h"
#include "../window.h"
#include "../core/discovery_engine.h"
#include "../core/mount_operation.h"

namespace Mounter {

DiscoveryPage::DiscoveryPage(Window& window)
  : Gtk::Box{Gtk::Orientation::VERTICAL}
  , window_(window)
{
  build_ui();
}

void DiscoveryPage::build_ui()
{
  set_margin(12);
  set_spacing(8);

  heading_.set_halign(Gtk::Align::START);
  heading_.get_style_context()->add_class("title-1");

  // ── Top bar ───────────────────────────────────────────────
  subnet_entry_.set_text(DiscoveryEngine::detect_subnet());
  subnet_entry_.set_width_chars(20);
  subnet_entry_.set_placeholder_text("192.168.1.0/24");
  subnet_entry_.set_tooltip_text("Subnet to scan in CIDR notation (e.g. 192.168.1.0/24)");

  scan_button_.get_style_context()->add_class("suggested-action");
  scan_button_.signal_clicked().connect(
    sigc::mem_fun(*this, &DiscoveryPage::on_scan_clicked));

  stop_button_.set_label(_("Stop"));
  stop_button_.set_visible(false);
  stop_button_.signal_clicked().connect([this]() {
    // TODO: wire cancel to the DiscoveryEngine instance
    progress_label_.set_text("Scan cancelled.");
    spinner_.stop();
    stop_button_.set_visible(false);
    scan_button_.set_visible(true);
  });

  progress_label_.set_halign(Gtk::Align::START);
  progress_label_.set_ellipsize(Pango::EllipsizeMode::END);
  progress_label_.set_hexpand(true);

  top_bar_.append(subnet_entry_);
  top_bar_.append(scan_button_);
  top_bar_.append(stop_button_);
  top_bar_.append(spinner_);
  top_bar_.append(progress_label_);

  // ── Results area ──────────────────────────────────────────
  placeholder_.set_halign(Gtk::Align::START);
  placeholder_.set_opacity(0.6);
  placeholder_.set_margin_top(12);

  results_list_.set_selection_mode(Gtk::SelectionMode::NONE);
  results_list_.get_style_context()->add_class("rich-list");

  scrolled_.set_child(results_list_);
  scrolled_.set_vexpand(true);
  scrolled_.set_visible(false);

  content_.set_vexpand(true);
  content_.append(top_bar_);
  content_.append(placeholder_);
  content_.append(scrolled_);

  append(heading_);
  append(content_);
}

void DiscoveryPage::on_scan_clicked()
{
  auto subnet = subnet_entry_.get_text();
  if (subnet.empty()) {
    window_.set_status("Please enter a subnet to scan.");
    return;
  }

  // UI feedback
  scan_button_.set_visible(false);
  stop_button_.set_visible(true);
  spinner_.start();
  progress_label_.set_text("Starting scan...");
  window_.set_status("Scanning " + subnet + " ...");

  // Clear previous results
  while (auto* child = results_list_.get_first_child()) {
    results_list_.remove(*child);
  }
  scrolled_.set_visible(false);
  placeholder_.set_visible(true);

  // The DiscoveryEngine is created on-demand for each scan
  auto engine = std::make_shared<DiscoveryEngine>();

  engine->scan_async(subnet,
    // Progress callback
    [this](const std::string& status) {
      progress_label_.set_text(status);
    },
    // Result callback
    [this, engine](const std::vector<DiscoveredHost>& hosts) {
      spinner_.stop();
      stop_button_.set_visible(false);
      scan_button_.set_visible(true);

      if (hosts.empty()) {
        progress_label_.set_text("No SMB hosts found on this subnet.");
        placeholder_.set_text("No SMB hosts found. Try a different subnet or check your network connection.");
        window_.set_status("Scan complete — no hosts found.");
      } else {
        size_t total_shares = 0;
        size_t smb_hosts = 0;
        for (const auto& h : hosts) {
          if (h.is_smb_server) smb_hosts++;
          total_shares += h.shares.size();
        }

        auto msg = "Found " + std::to_string(hosts.size()) + " host(s)";
        if (smb_hosts > 0) {
          msg += " (" + std::to_string(smb_hosts) + " SMB)";
        }
        msg += " with " + std::to_string(total_shares) + " share(s).";
        progress_label_.set_text(msg);
        window_.set_status("Scan complete.");

        on_results(hosts);
      }
    });
}

void DiscoveryPage::on_results(const std::vector<DiscoveredHost>& hosts)
{
  // Hide placeholder, show results
  placeholder_.set_visible(false);
  scrolled_.set_visible(true);

  // Clear existing
  while (auto* child = results_list_.get_first_child()) {
    results_list_.remove(*child);
  }

  for (const auto& host : hosts) {
    // ── Host header row ─────────────────────────────────────
    auto header_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
    header_row->set_margin_start(8);
    header_row->set_margin_end(8);
    header_row->set_margin_top(8);
    header_row->set_margin_bottom(4);

    // Host icon + name
    auto host_icon = Gtk::make_managed<Gtk::Label>(
      host.is_smb_server ? "\360\237\226\245" : "\342\232\240"); // 🖥 or ⚠
    host_icon->set_width_chars(2);

    std::string host_label = host.ip_address;
    if (!host.hostname.empty()) {
      host_label += " (" + host.hostname + ")";
    }

    auto host_name = Gtk::make_managed<Gtk::Label>(host_label);
    host_name->set_halign(Gtk::Align::START);
    host_name->get_style_context()->add_class("heading");
    host_name->set_hexpand(true);

    // Different badge based on SMB verification
    std::string count_text;
    if (host.is_smb_server) {
      count_text = std::to_string(host.shares.size()) + " share(s)";
    } else {
      count_text = "Port 445 open, not SMB";
    }
    auto share_count = Gtk::make_managed<Gtk::Label>(count_text);
    share_count->set_opacity(host.is_smb_server ? 0.6 : 0.4);

    header_row->append(*host_icon);
    header_row->append(*host_name);
    header_row->append(*share_count);
    results_list_.append(*header_row);

    // For non-SMB hosts, skip share rows entirely
    if (!host.is_smb_server) continue;

    // ── Share rows ──────────────────────────────────────────
    for (const auto& share : host.shares) {
      auto share_row = Gtk::make_managed<Gtk::Box>(Gtk::Orientation::HORIZONTAL, 8);
      share_row->set_margin_start(32); // indent
      share_row->set_margin_end(8);
      share_row->set_margin_top(2);
      share_row->set_margin_bottom(2);
      if (share.auth_required) {
        // Auth-required placeholder — show a lock icon and prompt
        auto lock_icon = Gtk::make_managed<Gtk::Label>("\360\237\224\222"); // 🔒
        lock_icon->set_width_chars(2);

        auto auth_label = Gtk::make_managed<Gtk::Label>(
          _("Authentication required"));
        auth_label->set_halign(Gtk::Align::START);
        auth_label->set_hexpand(true);
        auth_label->set_opacity(0.8);

        auto add_btn = Gtk::make_managed<Gtk::Button>(_("Add"));
        add_btn->get_style_context()->add_class("suggested-action");
        auto server = host.ip_address;
        auto hname  = host.hostname;
        add_btn->signal_clicked().connect([this, server, hname]() {
          window_.mount_page().prefill_from_discovery(server, hname);
        });

        share_row->append(*lock_icon);
        share_row->append(*auth_label);
        share_row->append(*add_btn);
        results_list_.append(*share_row);
        continue;
      }
      auto folder_icon = Gtk::make_managed<Gtk::Label>("\360\237\223\201"); // 📁
      folder_icon->set_width_chars(2);

      auto share_name = Gtk::make_managed<Gtk::Label>(share.name);
      share_name->set_halign(Gtk::Align::START);
      share_name->set_hexpand(true);

      // Access badge
      std::string badge_text = share.guest_ok ? "[Guest]" : "[Auth]";
      auto access_badge = Gtk::make_managed<Gtk::Label>(badge_text);
      access_badge->set_opacity(0.6);
      access_badge->set_width_chars(7);

      // Comment (if any)
      if (!share.comment.empty()) {
        auto comment_label = Gtk::make_managed<Gtk::Label>(share.comment);
        comment_label->set_opacity(0.5);
        comment_label->set_ellipsize(Pango::EllipsizeMode::END);
        comment_label->set_max_width_chars(25);
        share_row->append(*comment_label);
      }

      // Mount button
      auto mount_btn = Gtk::make_managed<Gtk::Button>(_("Mount"));
      mount_btn->get_style_context()->add_class("suggested-action");
      auto server = host.ip_address;
      auto share_name_copy = share.name;
      mount_btn->signal_clicked().connect([this, server, share_name_copy]() {
        on_mount_share(server, share_name_copy);
      });

      share_row->append(*folder_icon);
      share_row->append(*share_name);
      share_row->append(*access_badge);
      share_row->append(*mount_btn);
      results_list_.append(*share_row);
    }
  }
}

void DiscoveryPage::on_mount_share(const std::string& server,
                                   const std::string& share)
{
  // Quick mount with guest credentials — switch to manual mount page for auth
  // For guest shares, mount immediately
  MountParams params;
  params.server      = server;
  params.share       = share;
  params.mount_point = "/mnt/" + share;
  params.smb_version = "3.1.1";

  window_.set_status("Quick-mounting //" + server + "/" + share + " ...");

  window_.mount_operation().mount_async(params,
    [this, server, share](const MountResult& result) {
      if (result.success) {
        window_.set_status("Mounted //" + server + "/" + share);
      } else {
        window_.set_status(
          "Guest mount failed for //" + server + "/" + share +
          " — use Manual Mount tab for authenticated shares. Error: " +
          result.error_message);
      }
    });
}

} // namespace Mounter
