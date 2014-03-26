public_icons = \
	scalable_places_poi-airport.svg \
	scalable_places_poi-building.svg \
	scalable_places_poi-bus-stop.svg \
	scalable_places_poi-car.svg \
	scalable_places_poi-marker.svg \
	scalable_places_poi-railway-station.svg \
	scalable_places_poi-town.svg \
	scalable_places_poi-school.svg \
	scalable_places_poi-place-of-worship.svg \
	scalable_places_poi-restaurant.svg \
	scalable_places_poi-bar.svg \
	scalable_places_poi-airport.svg

gtk_update_icon_cache = gtk-update-icon-cache -f -t

install-data-local: install-icons
uninstall-local: uninstall-icons

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache

install-icons:
	-for icon in $(public_icons); do \
		ICON=`echo $$icon | sed -e 's:_:/:g'`; \
		ICONDIR=`dirname $$ICON`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/gnome/$$ICONDIR; \
		echo INSTALL $(DESTDIR)$(datadir)/icons/gnome/$$ICON; \
		$(INSTALL_DATA) $(srcdir)/maki/$$icon $(DESTDIR)$(datadir)/icons/gnome/$$ICON; \
	done;

uninstall-icons:
	-for icon in $(public_icons); do \
		ICON=`echo $$icon | sed -e 's:_:/:g'`; \
		echo rm -f $(DESTDIR)$(datadir)/icons/gnome/$$ICON; \
		rm -f $(DESTDIR)$(datadir)/icons/gnome/$$ICON; \
	done

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache) $(DESTDIR)$(datadir)/icons/gnome; \
	else \
		echo "*** Icon cache not updated. After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache) $(DESTDIR)$(datadir)/icons/gnome"; \
	fi

EXTRA_DIST = \
	maki/scalable_places_poi-airport.svg \
	maki/scalable_places_poi-building.svg \
	maki/scalable_places_poi-bus-stop.svg \
	maki/scalable_places_poi-car.svg \
	maki/scalable_places_poi-marker.svg \
	maki/scalable_places_poi-railway-station.svg \
	maki/scalable_places_poi-town.svg

-include $(top_srcdir)/git.mk
