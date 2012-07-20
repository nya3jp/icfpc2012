package jp.ac.utokyo.s.is.ui.phoenix.icfpc2012;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

public class MapData {
    private final List<String> lines;
    private final int width;
    private final int height;

    public MapData(File file) throws IOException {
        BufferedReader br = new BufferedReader(new FileReader(file));
        List<String> lines = new ArrayList<String>();
        int width = 0;
        while (true) {
            String line = br.readLine();
            if (line == null) {
                break;
            }
            width = Math.max(width, line.length());
            lines.add(line);
        }
        this.lines = lines;
        this.width = width;
        height = lines.size();
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public int get(int x, int y) {
        String line = lines.get(y);
        return x < line.length() ? line.charAt(x) : ' ';
    }
}
